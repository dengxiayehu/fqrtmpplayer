#ifndef _LIBFQRTMP_AAC_H_
#define _LIBFQRTMP_AAC_H_

#include "util.h"

jint openAudioEncoder(JNIEnv *env, jobject, jobject);
jint closeAudioEncoder(JNIEnv *env, jobject);
jint sendRawAudio(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len);

#endif /* end of _LIBFQRTMP_AAC_H_ */
