#include "utils.h"

const char LOG_TAG[] = "[native-opencv]";

inline void _log(const char *caller, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _log_(LOG_TAG, caller, fmt, args);
    va_end(args);
}

#define log(fmt, ...) _log(__FUNCTION__, fmt, ##__VA_ARGS__)

struct DetectFacesBaton {
	Mat *image;
	vector<Rect> *faces;
	uv_work_t request;
	Nan::Persistent<Function> callback;
	//char *err = NULL;
};

struct EncodeToJpegBaton {
	Mat *image;
	char *jpeg;
	int jpeg_len;
	int quality;
	uv_work_t request;
	Nan::Persistent<Function> callback;
	//char *err = NULL;
};

struct GrabImageBaton {
	Mat *image;
	uv_work_t request;
	Nan::Persistent<Function> callback;
	char *err = NULL;
};
