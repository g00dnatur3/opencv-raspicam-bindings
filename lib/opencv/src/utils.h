#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/face.hpp>
#include <raspicam/raspicam.h>
#include <stdarg.h>
#include <sys/time.h>
#include <v8.h>
#include <node.h>
#include <nan.h>
#include <stdio.h>
#include <string.h>
#include "node_pointer.h"

using namespace raspicam;
using namespace cv;
using namespace cv::face;
using namespace std;
using namespace v8;
using namespace node;

inline void _log_(const char *logTag, const char *caller, const char *fmt, va_list args) {
    fputs(logTag, stdout);
    fputs(" \0", stdout);
    fputs(caller, stdout);
    fputs(" - \0", stdout);
    vfprintf(stdout, fmt, args);
    fputs("\n\0", stdout);
}

inline int timeNowMillis() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec*1000 + tp.tv_usec/1000;
}

inline void makeErrorCallback(Local<Value> err, Local<Function> cb) {
	Local<Value> argv[1] = { err };
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, 1, argv);
}

inline char *toCharArray(Local<Value> jsValue) {
	Nan::Utf8String str(jsValue);
	return *str;
}

inline Mat gray(Mat image) {
	Mat gray;
	cvtColor(image, gray, COLOR_BGR2GRAY);
	return gray;
}

inline Rect toRect(Local<Object> jsRect, char *err) {
	Local<Value> x = jsRect->Get(Nan::New("x").ToLocalChecked());
	Local<Value> y = jsRect->Get(Nan::New("y").ToLocalChecked());
	Local<Value> width = jsRect->Get(Nan::New("width").ToLocalChecked());
	Local<Value> height = jsRect->Get(Nan::New("height").ToLocalChecked());
	if (!x->IsNumber() || !y->IsNumber() || !width->IsNumber() || !height->IsNumber()) {
		strcpy(err, "invalid Rect - x,y,width,height must all be integers");
		Rect rect;
		return rect;
	} else {
		err[0] = 0;
		return Rect(x->Int32Value(), y->Int32Value(), width->Int32Value(), height->Int32Value());
	}
}

inline vector<string> toStringArray(Handle<Array> jsArray) {
	vector<string> array;
	int len = jsArray->Length();
	array.reserve(len);
	for (int i = 0; i < len; i++) {
		array.push_back(string(toCharArray(jsArray->Get(i))));
	}
	return array;
}

inline vector<int> toIntArray(Handle<Array> jsArray) {
	vector<int> array;
	int len = jsArray->Length();
	array.reserve(len);
	for (int i = 0; i < len; i++) {
		array.push_back(jsArray->Get(i)->Int32Value());
	}
	return array;
}

inline vector<Mat> toImageArray(Handle<Array> jsArray) {
	vector<Mat> array;
	int len = jsArray->Length();
	array.reserve(len);
	for (int i = 0; i < len; i++) {
		Mat *image = reinterpret_cast<Mat *>(UnwrapPointer(jsArray->Get(i)));
		array.push_back(gray(*image));
	}
	return array;
}
