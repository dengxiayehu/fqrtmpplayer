#ifndef _NATIVE_CRASH_HANDLER_H_
#define _NATIVE_CRASH_HANDLER_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_native_crash_handler();
void destroy_native_crash_handler();

#ifdef __cplusplus
}
#endif
#endif /* end of _NATIVE_CRASH_HANDLER_H_ */
