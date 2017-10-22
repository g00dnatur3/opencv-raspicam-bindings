#include "bindings.h"

const int CAP_WIDTH = 1280;
const int CAP_HEIGHT = 720;

RaspiCam camera;
VideoCapture cap;

CascadeClassifier faceCascade;
double cvImageFormat;
Ptr<FaceRecognizer> model;

NAN_METHOD(OpenCamera) {
	if (camera.isOpened()) {
		log("error: camera already opened");
		return;
	}
	if (cap.isOpened()) {
		log("error: pipeline already opened");
		return;
	}
	camera.open() ? log("success") : log("error: malfunction");
}

NAN_METHOD(CloseCamera) {
	if (!camera.isOpened()) {
		log("error: not opened");
		return;
	}
	camera.release();
	log("success");
}

inline void grabImage(uv_work_t *req) {
	int ts = timeNowMillis();
	GrabImageBaton *baton = static_cast<GrabImageBaton *>(req->data);
	if (camera.isOpened()) {
		if (!camera.isOpened()) {
			baton->err = (char *) "error: camera not opened";
			log(baton->err); return;
		}
		if (!camera.grab()) {
			baton->err = (char *) "error: failed to read from camera";
			log(baton->err); return;
		}
		baton->image = new Mat(camera.getHeight(), camera.getWidth(), cvImageFormat);
		camera.retrieve(baton->image->ptr<uchar>(0));
	}
	else if (cap.isOpened()) {
		baton->image = new Mat;
		if (!cap.read(*baton->image)) {
			baton->err = (char *) "error: failed to read from pipeline";
			log(baton->err); delete baton->image; return;
		}
	}
	else log("error: camera/pipeline not open");
    log("completed in %d ms", timeNowMillis()-ts);
}

inline void grabImageAfter(uv_work_t *req, int status) {
	GrabImageBaton *baton = static_cast<GrabImageBaton *>(req->data);
	Nan::HandleScope scope;
	Local<Function> cb = Nan::New(baton->callback);
	if (baton->err != NULL) {
		log(baton->err);
		makeErrorCallback(Nan::New(baton->err).ToLocalChecked(), cb);
		return;
	}
	const unsigned argc = 2;
	Local<Value> argv[argc] = { Nan::Null(), WrapPointer(baton->image).ToLocalChecked() };
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
	baton->callback.Reset();
	delete baton;
}

NAN_METHOD(GrabImage) {
	GrabImageBaton *baton = new GrabImageBaton;
	baton->callback.Reset(info[0].As<Function>());
	baton->request.data = baton;
	uv_queue_work(uv_default_loop(), &baton->request, grabImage, grabImageAfter);
}

NAN_METHOD(ReleaseImage) {
	Mat *image = reinterpret_cast<Mat *>(UnwrapPointer(info[0]));
	image->release();
	delete image;
}

inline void encodeToJpeg(uv_work_t *req) {
	int ts = timeNowMillis();
	EncodeToJpegBaton *baton = static_cast<EncodeToJpegBaton *>(req->data);
	vector<int> params;
	params.push_back(CV_IMWRITE_JPEG_QUALITY);
	params.push_back(baton->quality);
	vector<uchar> buff;
	imencode(".jpg", *baton->image, buff, params);
	baton->jpeg = new char[buff.size()];
	copy(buff.begin(), buff.end(), baton->jpeg);
	baton->jpeg_len = buff.size();
	log("completed in %d ms", timeNowMillis()-ts);
}

inline void encodeToJpegAfter(uv_work_t *req, int status) {
	EncodeToJpegBaton *baton = static_cast<EncodeToJpegBaton *>(req->data);
	Nan::HandleScope scope;
	Local<Function> cb = Nan::New(baton->callback);
	Local<Value> jpeg = Nan::NewBuffer(baton->jpeg, baton->jpeg_len).ToLocalChecked();
	const unsigned argc = 2;
	Local<Value> argv[argc] = { Nan::Null(), jpeg };
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
	baton->callback.Reset();
	delete baton;
}

NAN_METHOD(EncodeToJpeg) {
	EncodeToJpegBaton *baton = new EncodeToJpegBaton;
	baton->image = reinterpret_cast<Mat *>(UnwrapPointer(info[0]));
	baton->quality = info[1]->Int32Value();
	baton->callback.Reset(info[2].As<Function>());
	baton->request.data = baton;
	uv_queue_work(uv_default_loop(), &baton->request, encodeToJpeg, encodeToJpegAfter);
}

NAN_METHOD(Crop) {
	Mat *image = reinterpret_cast<Mat *>(UnwrapPointer(info[0]));
	char err[60];
	Rect rect = toRect(info[1]->ToObject(), err);
	if (strlen(err) == 0) {
		Mat _image = *image;
		Mat *cropped = new Mat(rect.height, rect.width, image->type());
		_image(rect).copyTo(*cropped);
		info.GetReturnValue().Set(WrapPointer(cropped).ToLocalChecked());
	} else {
		log("error: %s", err);
		info.GetReturnValue().Set(Nan::Null());
	}
}

inline void detectFaces(uv_work_t *req) {
	int ts = timeNowMillis();
	DetectFacesBaton *baton = static_cast<DetectFacesBaton *>(req->data);
	Mat *image = baton->image;
	vector<Rect> *faces = new vector<Rect>;
	vector<Rect> &_faces = *faces;
	faceCascade.detectMultiScale(gray(*image), *faces, 1.1, 4, CASCADE_SCALE_IMAGE, Size(60,60), Size(200,200));
	//log("faces->size = %d", faces->size());
	int x;
	int y;
	int width;
	int height;
	int heighInc;
	int widthInc;
	for(size_t i=0; i<faces->size(); i++) {
		widthInc = _faces[i].width/4;
		heighInc = _faces[i].height/4;
		x = _faces[i].x - widthInc;
		y = _faces[i].y - heighInc;
		width = _faces[i].width + (2*widthInc);
		height = _faces[i].height + (2*heighInc);
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (width+x > CAP_WIDTH) width = CAP_WIDTH-x;
		if (height+y > CAP_HEIGHT) height = CAP_HEIGHT-y;
		_faces[i].x = x;
		_faces[i].y = y;
		_faces[i].width = width;
		_faces[i].height = height;
		//rectangle(*image, _faces[i], CV_RGB(255, 255 ,255), 1);
	}
	baton->faces = faces;
	log("completed in %d ms, %d faces detected", timeNowMillis()-ts, _faces.size());
}

inline void detectFacesAfter(uv_work_t *req, int status) {
	DetectFacesBaton *baton = static_cast<DetectFacesBaton *>(req->data);
	Nan::HandleScope scope;
	Local<Function> cb = Nan::New(baton->callback);
	vector<Rect> &faces = *baton->faces;
	Local<Array> jsFaces = Nan::New<Array>();
	for(size_t i=0; i<faces.size(); i++) {
		Local<Object> jsFace = Nan::New<Object>();
		jsFace->Set(Nan::New("x").ToLocalChecked(), Nan::New<Int32>(faces[i].x));
		jsFace->Set(Nan::New("width").ToLocalChecked(), Nan::New<Int32>(faces[i].width));
		jsFace->Set(Nan::New("y").ToLocalChecked(), Nan::New<Int32>(faces[i].y));
		jsFace->Set(Nan::New("height").ToLocalChecked(), Nan::New<Int32>(faces[i].height));
		jsFaces->Set(i, jsFace);
	}
	faces.clear();
	const unsigned argc = 2;
	Local<Value> argv[argc] = { Nan::Null(), jsFaces };
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
	baton->callback.Reset();
	delete baton;
}

NAN_METHOD(DetectFaces) {
	DetectFacesBaton *baton = new DetectFacesBaton;
	baton->image = reinterpret_cast<Mat *>(UnwrapPointer(info[0]));
	baton->callback.Reset(info[1].As<Function>());
	baton->request.data = baton;
	uv_queue_work(uv_default_loop(), &baton->request, detectFaces, detectFacesAfter);
}

void init(Local<Object> target, Handle<Object> module) {

	Nan::HandleScope scope;

	Nan::SetMethod(target, "openCamera", OpenCamera);
	Nan::SetMethod(target, "closeCamera", CloseCamera);
	Nan::SetMethod(target, "grabImage", GrabImage);
	Nan::SetMethod(target, "releaseImage", ReleaseImage);
	Nan::SetMethod(target, "detectFaces", DetectFaces);
	Nan::SetMethod(target, "encodeToJpeg", EncodeToJpeg);
	Nan::SetMethod(target, "crop", Crop);

	camera.setFormat(RASPICAM_FORMAT_BGR);
    if (camera.getFormat() == RASPICAM_FORMAT_BGR) {
    	cvImageFormat = CV_8UC3;
    } else if (camera.getFormat() == RASPICAM_FORMAT_GRAY) {
    	cvImageFormat = CV_8UC1;
    } else {
		log("error: invalid camera format");
    }
    camera.setCaptureSize(CAP_WIDTH, CAP_HEIGHT);

    //get the absolute path of this module, use it to resolve other paths
    Local<Value> filename = module->Get(Nan::New("filename").ToLocalChecked());

    // resolve the absolute path of the cascade classifier we will use.
    Local<Function> require = Local<Function>::Cast(module->Get(Nan::New("require").ToLocalChecked()));
	Local<Value> pathArgs[1] = { Nan::New("path").ToLocalChecked() };
	Local<Object> pathModule = Nan::MakeCallback(target, require, 1, pathArgs)->ToObject();
	Local<Function> resolve = Local<Function>::Cast(pathModule->Get(Nan::New("resolve").ToLocalChecked()));
	Local<Value> resolveArgs[2] = { filename, Nan::New(
			"../../../cascade_classifiers/lbpcascade_frontalface.xml").ToLocalChecked() };
	Local<Value> lbpcascadeFrontalface = Nan::MakeCallback(target, resolve, 2, resolveArgs);

	faceCascade.load(toCharArray(lbpcascadeFrontalface));
	model = createLBPHFaceRecognizer(10);
}

NODE_MODULE(bindings, init)
