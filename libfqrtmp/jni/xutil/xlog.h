#ifndef _XLOG_H_
#define _XLOG_H_

#include <libgen.h>
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "FQRtmp"
#endif

#define V(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_VERBOSE, LOG_TAG, fmt, ##__VA_ARGS__)
#define D(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define I(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define W(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_WARN, LOG_TAG, fmt, ##__VA_ARGS__)
#define E(fmt, ...) libfqrtmp_log_print(basename(__FILE__), __LINE__, ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

static inline int libfqrtmp_log_print(const char *file, const int line,
                                      const android_LogPriority prio, const char *tag,
                                      const char *fmt, ...)
{
    char buf[4096];
    int ret;

    ret = snprintf(buf, 4096, "[%s:%04d] ", file, line);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf + ret, 4096 - ret, fmt, args);
    va_end(args);

    __android_log_write(prio, tag, buf);
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* end of _XLOG_H_ */
