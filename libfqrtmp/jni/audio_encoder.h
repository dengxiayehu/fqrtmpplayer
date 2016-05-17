#ifndef _AUDIO_ENCODER_H_
#define _AUDIO_ENCODER_H_

#include <jni.h>
#include <fdk-aac/aacenc_lib.h>

#include "xutil.h"
#include "xfile.h"

#ifdef __cplusplus
extern "C" {
#endif

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();

    int init(jobject audio_config);
    int feed(uint8_t *buffer, int len);
    volatile bool quit() const;

private:
    DISALLOW_COPY_AND_ASSIGN(AudioEncoder);
    xutil::Frac m_pts;
    HANDLE_AACENCODER m_hdlr;
    AACENC_InfoStruct m_info;
    int m_aot;
    int m_samplerate;
    int m_channels;
    int m_bits_per_sample;
    xutil::RecursiveMutex m_mutex;
    xutil::Condition m_cond;
    DECL_THREAD_ROUTINE(AudioEncoder, encode_routine);
    xutil::Thread *m_thrd;
    xutil::IOBuffer *m_iobuf;
    volatile bool m_quit;
    xfile::File *m_file;
};

jint openAudioEncoder(JNIEnv *env, jobject, jobject);
jint closeAudioEncoder(JNIEnv *env, jobject);
jint sendRawAudio(JNIEnv *env, jobject thiz, jbyteArray byte_arr, jint len);

#ifdef __cplusplus
}
#endif
#endif /* end of _AUDIO_ENCODER_H_ */
