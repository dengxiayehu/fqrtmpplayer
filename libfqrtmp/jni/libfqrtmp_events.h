#ifndef _LIBFQRTMP_EVENTS_H_
#define _LIBFQRTMP_EVENTS_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OPENING,
    CONNECTED,
    PLAYING,
    PUSHING,
    ENCOUNTERED_ERROR,
    NUM_EVENT
} libfqrtmp_event;

void libfqrtmp_event_send(libfqrtmp_event type, jlong arg0, jstring arg2);

#ifdef __cplusplus
}
#endif

#endif /* end of _LIBFQRTMP_EVENTS_H_ */
