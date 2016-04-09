#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "FQRtmp"
#endif

#define LOGV(fmt, ...) libfqrtmp_log_print(__FILE__, __LINE__, ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) libfqrtmp_log_print(__FILE__, __LINE__, ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) libfqrtmp_log_print(__FILE__, __LINE__, ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) libfqrtmp_log_print(__FILE__, __LINE__, ANDROID_LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) libfqrtmp_log_print(__FILE__, __LINE__, ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

#define NELEM(arr)  (sizeof(arr)/sizeof(arr[0]))

#ifdef __cplusplus
extern "C" {
#endif

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
};

extern struct LibFQRtmp LibFQRtmp;

jstring my_new_string(const char *str);

static inline int libfqrtmp_log_print(const char *file, const int line,
                                      const android_LogPriority prio, const char *tag,
                                      const char *fmt, ...)
{
    char buf[4096];
    int ret;

    ret = snprintf(buf, 4096, "[%s:%d] ", file, line);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf + ret, 4096 - ret, fmt, args);
    va_end(args);

    __android_log_write(prio, tag, buf);
    return 0;
}

static inline void throw_IllegalArgumentException(JNIEnv *env, const char *error)
{
    (*env)->ThrowNew(env, LibFQRtmp.IllegalArgumentException.clazz, error);
}

#ifdef __cplusplus
}
#endif
#endif /* end of _UTIL_H_ */
