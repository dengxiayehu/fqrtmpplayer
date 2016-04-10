#include "util.h"

#define THREAD_NAME "util"
extern JNIEnv *jni_get_env(const char *name);

jstring new_string(const char *str)
{
    JNIEnv *env = jni_get_env(THREAD_NAME);
    jmethodID cid;
    jbyteArray arr;
    jsize len;
    jstring result;

    cid = (*env)->GetMethodID(env, LibFQRtmp.String.clazz,
                              "<init>", "([B)V");
    if (!cid)
        return NULL;

    len = strlen(str);
    arr = (*env)->NewByteArray(env, len);
    if (!arr)
        return NULL;
    (*env)->SetByteArrayRegion(env, arr, 0, len, (const jbyte *) str);

    result = (*env)->NewObject(env, LibFQRtmp.String.clazz, cid, arr);

    (*env)->DeleteLocalRef(env, arr);
    return result;
}

void rtmp_log(int level, const char *fmt, va_list args)
{
    if (level == RTMP_LOGDEBUG2)
        return;

    char buf[4096];
    vsnprintf(buf, sizeof(buf)-1, fmt, args);

    android_LogPriority prio;

    switch (level) {
        default:
        case RTMP_LOGCRIT:
        case RTMP_LOGERROR:     prio = ANDROID_LOG_ERROR; break;
        case RTMP_LOGWARNING:   prio = ANDROID_LOG_WARN;  break;
        case RTMP_LOGINFO:      prio = ANDROID_LOG_INFO;  break;
        case RTMP_LOGDEBUG:     prio = ANDROID_LOG_DEBUG; break;
    }

    libfqrtmp_log_print("rtmp_module", -1, prio, LOG_TAG, buf);
}
