# Raspicam + OpenCV Bindings for NodeJS

Example to demonstrate how to do NodeJS Binding With OpenCV.

## Requirements

You need a Raspberry Pi with a camera Module.

You need to have the following software compiled and installed:

* NodeJS
* OpenCV 3.2
* raspicam 0.1.6 `https://www.uco.es/investiga/grupos/ava/node/40`

I included scripts to download and build those dependencies.

Please take a look  in the `/build-scripts` directory.

## Usage

To run the example do this:

> node test/example.js

The example will keep grabbing a images and trying to detect a face.
The first face that is detected will be cropped out and saved to sample.jpg.

## OpenCV Binding Info

The binding uses the `lbpcascade_frontalface.xml` classifier to detect faces.
A face is not detected 100% of the time, there is false positives around 10-20% of the time.

## Binding API

	`openCamera` : opens the camera, does not take any parameters, is synchronous.
	
	`closeCamera`: closes the camera, does not take any parameters, is synchronous.
	
	'grabImage'  : grabs and returns a pointer to an image asynchronously

		Example:
		```
		opencv.grabImage(function(err, image) {
			if (!err) {
				//image -> pointer to OpenCV Mat object
			}
		});
		```

## Sample Code

```
const log = require('node-log').log();
const opencv = require('../lib/opencv');
const async = require('async');

//
//	This example detects a face, crops it, and saves it as sample.jpg
//

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
```



