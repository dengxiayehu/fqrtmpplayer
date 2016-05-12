#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <jni.h>
#include <librtmp/log.h>
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "FQRtmp"
#endif

#define V(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#define D(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define I(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define W(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define E(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

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

jstring new_string(const char *str);

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

#define ERRNOMSG strerror_(errno)
static inline const char *strerror_(int err)
{
    int errno_save = errno;
    const char *retval = strerror(err);
    errno = errno_save;
    return retval;
}

#ifndef SAFE_FREE
#define SAFE_FREE(p) do { \
    free(p);              \
    p = NULL;             \
} while (0)
#endif

#define BEGIN   {
#define END     }

void rtmp_log(int level, const char *fmt, va_list args);

static inline uint8_t *put_be16(uint8_t *output, uint16_t val)
{        
    output[1] = val & 0xff;
    output[0] = val >> 8;
    return output + 2;
}

static inline uint8_t *put_be24(uint8_t *output, uint32_t val)
{
    output[2] = val & 0xff;
    output[1] = val >> 8;
    output[0] = val >> 16;
    return output + 3;
}

static inline uint8_t *put_be32(uint8_t *output, uint32_t val)
{
    output[3] = val & 0xff;
    output[2] = val >> 8;
    output[1] = val >> 16;
    output[0] = val >> 24;
    return output + 4;
}

static inline uint8_t *put_be64(uint8_t *output, uint64_t val)
{   
    output = put_be32(output, val >> 32);
    output = put_be32(output, (uint32_t) val); 
    return output;
}

#ifdef __cplusplus
}
#endif
#endif /* end of _UTIL_H_ */
