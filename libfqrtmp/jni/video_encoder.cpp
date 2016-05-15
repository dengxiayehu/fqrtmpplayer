#include "video_encoder.h"

VideoEncoder::VideoEncoder()
{
}

VideoEncoder::~VideoEncoder()
{
}

int VideoEncoder::init(jobject video_config)
{
    return 0;
}

int VideoEncoder::feed(uint8_t *buffer, int len)
{
    return 0;
}

jint openVideoEncoder(JNIEnv *env, jobject, jobject)
{
    return 0;
}

jint closeVideoEncoder(JNIEnv *env, jobject)
{
    return 0;
}

jint sendRawVideo(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len)
{
    return 0;
}
