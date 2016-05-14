#include "libfqrtmp_events.h"
#include "util.h"

#include <stdio.h>

#define THREAD_NAME "libfqrtmp_events"
extern JNIEnv *jni_get_env(const char *name);

void libfqrtmp_event_send(libfqrtmp_event type, jlong arg1, jstring arg2)
{
    JNIEnv *env = NULL;

    if (!(env = jni_get_env(THREAD_NAME)))
       return;

    if (!env->IsSameObject(LibFQRtmp.weak_thiz, NULL)) {
        env->CallVoidMethod(LibFQRtmp.weak_thiz,
                            LibFQRtmp.dispatchEventFromNativeID, (jint) type, arg1, arg2);
    }
}
