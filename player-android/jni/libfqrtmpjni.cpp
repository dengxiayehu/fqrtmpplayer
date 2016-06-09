#include <jni.h>
#include <getopt.h>
#include <pthread.h>

#include "native_crash_handler.h"
#include "libfqrtmp_events.h"
#include "rtmp_handler.h"
#include "audio_encoder.h"
#include "video_encoder.h"
#include "common.h"
#include "config.h"
#include "xutil.h"

struct LibFQRtmp gfq;

#define JNI_VERSION JNI_VERSION_1_2

#define THREAD_NAME "libfqrtmpjni"

static JavaVM *cached_jvm;

static pthread_key_t jni_env_key;

static jstring version(JNIEnv *, jobject);
static void nativeNew(JNIEnv *, jobject, jstring cmdline);
static void nativeRelease(JNIEnv *, jobject);

static JNINativeMethod method[] = {
    {"version", "()Ljava/lang/String;", (void *) version},
    {"nativeNew", "(Ljava/lang/String;)V", (void *) nativeNew},
    {"nativeRelease", "()V", (void *) nativeRelease},
    {"sendRawAudio", "([BI)I", (void *) sendRawAudio},
    {"sendRawVideo", "([BII)I", (void *) sendRawVideo},
    {"openAudioEncoder", "(Lcom/dxyh/libfqrtmp/LibFQRtmp$AudioConfig;)I", (void *) openAudioEncoder},
    {"closeAudioEncoder", "()I", (void *) closeAudioEncoder},
    {"openVideoEncoder", "(Lcom/dxyh/libfqrtmp/LibFQRtmp$VideoConfig;)I", (void *) openVideoEncoder},
    {"closeVideoEncoder", "()I", (void *) closeVideoEncoder},
};

static void jni_detach_thread(void *data)
{
    cached_jvm->DetachCurrentThread();
}

JNIEnv *jni_get_env(const char *name)
{
    JNIEnv *env;

    env = (JNIEnv *) pthread_getspecific(jni_env_key);
    if (!env) {
        if (cached_jvm->GetEnv((void **) &env, JNI_VERSION) != JNI_OK) {
            JavaVMAttachArgs args;

            args.version = JNI_VERSION;
            args.name = name;
            args.group = NULL;

            if (cached_jvm->AttachCurrentThread(&env, &args) != JNI_OK)
                return NULL;

            if (pthread_setspecific(jni_env_key, env) != 0) {
                cached_jvm->DetachCurrentThread();
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

    if (jvm->GetEnv((void **) &env, JNI_VERSION) != JNI_OK)
        return JNI_ERR;

    if (pthread_key_create(&jni_env_key, jni_detach_thread) != 0)
        return JNI_ERR;

#define GET_CLASS(clazz, str, globlal) do { \
    (clazz) = env->FindClass((str)); \
    if (!(clazz)) { \
        E("FindClass(%s) failed", (str)); \
        return -1; \
    } \
    if (globlal) { \
        (clazz) = (jclass) env->NewGlobalRef((clazz)); \
        if (!(clazz)) { \
            E("NewGlobalRef(%s) failed", (str)); \
            return -1; \
        } \
    } \
} while (0)

#define GET_ID(get, id, clazz, str, args) do { \
    (id) = env->get((clazz), (str), (args)); \
    if (!(id)) { \
        E(#get"(%s) failed", (str)); \
        return -1; \
    } \
} while (0)

    GET_CLASS(gfq.clazz,
              "com/dxyh/libfqrtmp/LibFQRtmp", 1);

    GET_CLASS(gfq.IllegalArgumentException.clazz,
              "java/lang/IllegalArgumentException", 1);

    GET_CLASS(gfq.String.clazz,
              "java/lang/String", 1);

    GET_ID(GetStaticMethodID,
           gfq.onNativeCrashID,
           gfq.clazz,
           "onNativeCrash", "()V");

    GET_ID(GetMethodID,
           gfq.dispatchEventFromNativeID,
           gfq.clazz,
           "dispatchEventFromNative", "(IJLjava/lang/String;)V");

    env->RegisterNatives(gfq.clazz, method, NELEM(method));

    init_native_crash_handler();

    I("JNI interface loaded.");
    return JNI_VERSION;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved)
{
    JNIEnv *env;

    destroy_native_crash_handler();

    if (jvm->GetEnv((void **) &env, JNI_VERSION))
        return;

    env->DeleteGlobalRef(gfq.clazz);
    env->DeleteGlobalRef(gfq.String.clazz);
    env->DeleteGlobalRef(gfq.IllegalArgumentException.clazz);

    pthread_key_delete(jni_env_key);
}

static jstring version(JNIEnv *env, jobject thiz)
{
    return jnu_new_string(VERSION_MESSAGE);
}

static std::string liveurl;
static std::string flvpath;

static int parse_arg(const char *str)
{
    int n = 3, argc = 0;
    const char **argv =
        (const char **) malloc(n * sizeof(const char *));
    char *cmdline = strdup(str);
    const char *delim = " ";
    const char *p = strtok(cmdline, delim);
    int retval = 0;

    argv[argc++] = "libfqrtmp";
    while (p) {
        if (argc == n)
           argv = (const char **) realloc(argv, (n+=2)*sizeof(const char *));
        argv[argc++] = p;
        p = strtok(NULL, delim);
    }

    BEGIN
    struct option longopts[] = {
        {"live",    required_argument, NULL, 'L'},
        {"flvpath", required_argument, NULL, 'f'},
        {0, 0, 0, 0}
    };
    int ch;

    optind = 0;
    while ((ch = getopt_long(argc, (char * const *) argv,
                             ":L:f:W;", longopts, NULL)) != -1) {
        switch (ch) {
        case 'L':
            liveurl = optarg;
            break;

        case 'f':
            flvpath = optarg;
            break;

        case 0:
            break;

        case '?':
        default:
            E("Unknown option: %c", optopt);
            retval = -1;
            goto out;
        }
    }
    END

out:
    SAFE_FREE(argv);
    SAFE_FREE(cmdline);
    return retval;
}

static void nativeNew(JNIEnv *env, jobject thiz, jstring cmdline)
{
    const char *str;

    str = env->GetStringUTFChars(cmdline, NULL);
    if (!str) {
        throw_IllegalArgumentException(env, "cmdline invalid");
        return;
    }

    if (parse_arg(str) < 0) {
        E("parse_arg failed");
        goto out;
    }

    gfq.weak_thiz = env->NewWeakGlobalRef(thiz);
    if (!gfq.weak_thiz) {
        E("Create weak-reference for libfqrtmp instance failed");
        goto out;
    }

    libfqrtmp_event_send(OPENING, 0, jnu_new_string(""));

    gfq.rtmp_hdlr = new RtmpHandler(flvpath);
    if (gfq.rtmp_hdlr->connect(liveurl) < 0) {
        libfqrtmp_event_send(ENCOUNTERED_ERROR,
                             -1001, jnu_new_string("rtmp_connect failed"));
        goto out;
    }

    libfqrtmp_event_send(CONNECTED, 0, jnu_new_string(""));

out:
    env->ReleaseStringUTFChars(cmdline, str);
}

static void nativeRelease(JNIEnv *env, jobject thiz)
{
    SAFE_DELETE(gfq.rtmp_hdlr);

    if (!env->IsSameObject(gfq.weak_thiz, NULL)) {
        env->DeleteWeakGlobalRef(gfq.weak_thiz);
    }
}
