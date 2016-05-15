#ifndef _VIDEO_ENCODER_H_
#define _VIDEO_ENCODER_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

jint openVideoEncoder(JNIEnv *env, jobject, jobject);
jint closeVideoEncoder(JNIEnv *env, jobject);
jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len);

#ifdef __cplusplus
}
#endif
#endif /* end of _VIDEO_ENCODER_H_ */
