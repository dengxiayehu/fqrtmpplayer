LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rtmp

LOCAL_SRC_FILES := ../../contrib/install/lib/librtmp.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

CONTRIB_INSTALL := $(LOCAL_PATH)/../../contrib/install
PRIVATE_INCDIR := $(CONTRIB_INSTALL)/include
PRIVATE_LIBDIR := $(CONTRIB_INSTALL)/lib

LOCAL_MODULE := libfqrtmpjni

LOCAL_SRC_FILES := libfqrtmpjni.c native_crash_handler.c util.c

LOCAL_C_INCLUDES := $(PRIVATE_INCDIR)
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := rtmp
include $(BUILD_SHARED_LIBRARY)
