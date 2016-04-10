#include <jni.h>
#include <pthread.h>

#include "native_crash_handler.h"
#include "libfqrtmp_events.h"
#include "rtmp.h"
#include "util.h"
#include "config.h"

struct LibFQRtmp LibFQRtmp;

#define JNI_VERSION JNI_VERSION_1_2

#define THREAD_NAME "libfqrtmpjni"

static rtmp_t *r;
static JavaVM *cached_jvm;

static pthread_key_t jni_env_key;

static jstring version(JNIEnv *, jobject);
static void nativeNew(JNIEnv *, jobject, jstring cmdline);
static void nativeRelease(JNIEnv *, jobject);

static JNINativeMethod method[] = {
    {"version", "()Ljava/lang/String;", (void *) version},
    {"nativeNew", "(Ljava/lang/String;)V", (void *) nativeNew},
    {"nativeRelease", "()V", (void *) nativeRelease},
};

static void jni_detach_thread(void *data)
{
    (*cached_jvm)->DetachCurrentThread(cached_jvm);
}

JNIEnv *jni_get_env(const char *name)
{
    JNIEnv *env;

    env = pthread_getspecific(jni_env_key);
    if (!env) {
        if ((*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION) != JNI_OK) {
            JavaVMAttachArgs args;
            jint result;

            args.version = JNI_VERSION;
            args.name = name;
            args.group = NULL;

            if ((*cached_jvm)->AttachCurrentThread(cached_jvm, &env, &args) != JNI_OK)
                return NULL;

            if (pthread_setspecific(jni_env_key, env) != 0) {
                (*cached_jvm)->DetachCurrentThread(cached_jvm);
                return NULL;
            }
        }
    }

    return env;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *env = NULL;
    cached_jvm = jvm;

    if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION) != JNI_OK)
        return JNI_ERR;

    if (pthread_key_create(&jni_env_key, jni_detach_thread) != 0)
        return JNI_ERR;

#define GET_CLASS(clazz, str, globlal) do { \
    (clazz) = (*env)->FindClass(env, (str)); \
    if (!(clazz)) { \
        E("FindClass(%s) failed", (str)); \
        return -1; \
    } \
    if (globlal) { \
        (clazz) = (jclass) (*env)->NewGlobalRef(env, (clazz)); \
        if (!(clazz)) { \
            E("NewGlobalRef(%s) failed", (str)); \
            return -1; \
        } \
    } \
} while (0)

#define GET_ID(get, id, clazz, str, args) do { \
    (id) = (*env)->get(env, (clazz), (str), (args)); \
    if (!(id)) { \
        E(#get"(%s) failed", (str)); \
        return -1; \
    } \
} while (0)

    GET_CLASS(LibFQRtmp.clazz,
              "com/dxyh/libfqrtmp/LibFQRtmp", 1);

    GET_CLASS(LibFQRtmp.IllegalArgumentException.clazz,
              "java/lang/IllegalArgumentException", 1);

    GET_CLASS(LibFQRtmp.String.clazz,
              "java/lang/String", 1);

    GET_ID(GetStaticMethodID,
           LibFQRtmp.onNativeCrashID,
           LibFQRtmp.clazz,
           "onNativeCrash", "()V");

    GET_ID(GetMethodID,
           LibFQRtmp.dispatchEventFromNativeID,
           LibFQRtmp.clazz,
           "dispatchEventFromNative", "(IJLjava/lang/String;)V");

    (*env)->RegisterNatives(env, LibFQRtmp.clazz, method, NELEM(method));

    init_native_crash_handler();

    E("JNI interface loaded.");
    return JNI_VERSION;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    JNIEnv *env;

    destroy_native_crash_handler();

    if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION))
        return;

    (*env)->DeleteGlobalRef(env, LibFQRtmp.clazz);
    (*env)->DeleteGlobalRef(env, LibFQRtmp.String.clazz);
    (*env)->DeleteGlobalRef(env, LibFQRtmp.IllegalArgumentException.clazz);

    pthread_key_delete(jni_env_key);
}

static jstring version(JNIEnv *env, jobject thiz)
{
    return new_string(VERSION_MESSAGE);
}

static void nativeNew(JNIEnv *env, jobject thiz, jstring cmdline)
{
    const char *str;

    libfqrtmp_event_send(OPENING, 0, new_string(""));

    str = (*env)->GetStringUTFChars(env, cmdline, NULL);
    if (!str) {
        throw_IllegalArgumentException(env, "cmdline invalid");
        return;
    }

    LibFQRtmp.weak_thiz = (*env)->NewWeakGlobalRef(env, thiz);
    if (!LibFQRtmp.weak_thiz) goto out;

    r = rtmp_init(str);
    if (r) {
        if (rtmp_connect(r) < 0)
            libfqrtmp_event_send(ENCOUNTERED_ERROR,
                                 -1001, new_string("rtmp_connect failed"));
        else
            libfqrtmp_event_send(CONNECTED, 0, new_string(""));
    } else
        libfqrtmp_event_send(ENCOUNTERED_ERROR,
                             -1001, new_string("rtmp_init failed"));

out:
    (*env)->ReleaseStringUTFChars(env, cmdline, str);
}

static void nativeRelease(JNIEnv *env, jobject thiz)
{
    if (r)
        rtmp_disconnect(r);

    (*env)->DeleteWeakGlobalRef(env, LibFQRtmp.weak_thiz);
}
