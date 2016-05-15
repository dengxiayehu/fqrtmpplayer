#ifndef _VIDEO_ENCODER_H_
#define _VIDEO_ENCODER_H_

#include <jni.h>
#include <stdint.h>
#include <x264.h>

#include "xutil.h"

#ifdef __cplusplus
extern "C" {
#endif

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    int init(jobject video_config);
    int feed(uint8_t *buffer, int len);
};

jint openVideoEncoder(JNIEnv *env, jobject, jobject);
jint closeVideoEncoder(JNIEnv *env, jobject);
jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len);

#ifdef __cplusplus
}
#endif
#endif /* end of _VIDEO_ENCODER_H_ */
