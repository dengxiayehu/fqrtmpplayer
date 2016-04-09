#include "util.h"

#define THREAD_NAME "util"
extern JNIEnv *jni_get_env(const char *name);

jstring my_new_string(const char *str)
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
