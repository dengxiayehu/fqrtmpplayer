#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <jni.h>
#include <librtmp/log.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Rational {
    int num;
    int den;
};

struct Packet {
    uint8_t *data;
    int size;
    int64_t pts, dts;
    int duration;

    explicit Packet(uint8_t *data_ = NULL, int size_ = 0, int pts_ = 0, int dts_ = 0, int duration_ = 0);
    ~Packet();
};

class AudioEncoder;
class VideoEncoder;

struct LibFQRtmp {
    jclass clazz;
    jobject weak_thiz;
    struct {
        jclass clazz;
    } IllegalArgumentException;
    struct {
        jclass clazz;
    } String;
    jmethodID onNativeCrashID;
    jmethodID dispatchEventFromNativeID;
    AudioEncoder *audio_enc;
    VideoEncoder *video_enc;
};

extern struct LibFQRtmp gfq;

jstring jnu_new_string(const char *str);

static inline void throw_IllegalArgumentException(JNIEnv *env, const char *error)
{
    env->ThrowNew(gfq.IllegalArgumentException.clazz, error);
}

jvalue jnu_get_field_by_name(jboolean *has_exception, jobject obj,
                             const char *name, const char *signature);
void jnu_set_field_by_name(jboolean *hasException, jobject obj,
                           const char *name, const char *signature, ...);
jvalue jnu_call_method_by_name(jboolean *has_exception, jobject obj,
                               const char *name, const char *signature, ...);
jvalue jnu_call_method_by_name_v(jboolean *has_exception, jobject obj,
                                 const char *name, const char *signature, va_list args);

void rtmp_log(int level, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif
#endif /* end of _COMMON_H_ */
