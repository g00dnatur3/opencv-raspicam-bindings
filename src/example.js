const log = require('node-log').log();
const opencv = require('../lib/opencv');
const async = require('async');



opencv.openCamera();

function grabCroppedFaceAsJpeg(onComplete) {
	async.waterfall([
		opencv.grabImage,
  	    function (image, callback) {
  	    	opencv.detectFaces(image, function(err, faces) {
  	    		if (err) callback(err);
  	    		else callback(null, image, faces);
  	    	});
  	    },
  	    function (image, faces, callback) {
  	    	if (faces.length === 0) {
  	    		setImmediate(grabCroppedFaceAsJpeg.bind(null, onComplete));
  	    		callback('ok'); //quietly exit this waterfall
  	    	} else {
  	    		const cropped = opencv.crop(image, faces[0]);
  	    		callback(null, cropped);
  	    	}
  	    	opencv.releaseImage(image);
  	    },
  	    function (cropped, callback) {
	  	  	opencv.encodeToJpeg(cropped, 94, function(err, data) {
	  			opencv.releaseImage(cropped);
	  			callback(err, data);
	  		});
  	    }
  	], function(err) {
 		if (err === 'ok') return;
 		else onComplete.apply(null, [].slice.call(arguments));
 	});
}

const path = require("path");
const samplePath = path.resolve("sample.jpg"); 

grabCroppedFaceAsJpeg(function(err, jpeg) {
	if (!err) {
		const fs = require('fs');
		fs.createWriteStream(samplePath).end(jpeg);
		log('successfully cropped a face and saved it to: ' + samplePath);
	} else {
		log('err: ' + err);
	}
	opencv.closeCamera();
});