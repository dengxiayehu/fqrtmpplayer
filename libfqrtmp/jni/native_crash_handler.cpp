#include "native_crash_handler.h"
#include "common.h"
#include "xutil.h"

static struct sigaction old_actions[NSIG];

#define THREAD_NAME "native_crash_handler"
extern JNIEnv *jni_get_env(const char *name);

static const int monitored_signals[] = {
    SIGILL,
    SIGABRT,
    SIGBUS,
    SIGFPE,
    SIGSEGV,
    SIGSTKFLT,
    SIGPIPE
};

void sigaction_callback(int signal, siginfo_t *info, void *reserved)
{
    JNIEnv *env;
    if (!(env = jni_get_env(THREAD_NAME)))
        return;

    env->CallStaticVoidMethod(gfq.clazz, gfq.onNativeCrashID);

    old_actions[signal].sa_handler(signal);
}

void init_native_crash_handler()
{
    unsigned i;
    struct sigaction handler;
    memset(&handler, 0, sizeof(struct sigaction));

    handler.sa_sigaction = sigaction_callback;
    handler.sa_flags = SA_RESETHAND;

    for (i = 0; i < NELEM(monitored_signals); ++i) {
        const int s = monitored_signals[i];
        sigaction(s, &handler, &old_actions[s]);
    }
}

void destroy_native_crash_handler()
{
    unsigned i;
    for (i = 0; i < NELEM(monitored_signals); ++i) {
        const int s = monitored_signals[i];
        sigaction(s, &old_actions[s], NULL);
    }
}
