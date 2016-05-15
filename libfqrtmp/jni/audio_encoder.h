#ifndef _AUDIO_ENCODER_H_
#define _AUDIO_ENCODER_H_

#include <jni.h>
#include <fdk-aac/aacenc_lib.h>

#include "xutil.h"

#ifdef __cplusplus
extern "C" {
#endif

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();

    int init(jobject audio_config);

private:
    HANDLE_AACENCODER m_hdlr;
    AACENC_InfoStruct m_info;
    int m_aot;
    int m_samplerate;
    int m_channels;
    int m_bits_per_sample;
    int16_t *m_convert_buf;
};

jint openAudioEncoder(JNIEnv *env, jobject, jobject);
jint closeAudioEncoder(JNIEnv *env, jobject);
jint sendRawAudio(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len);

#ifdef __cplusplus
}
#endif
#endif /* end of _AUDIO_ENCODER_H_ */
