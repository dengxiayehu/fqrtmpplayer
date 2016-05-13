LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rtmp

LOCAL_SRC_FILES := ../../contrib/install/lib/librtmp.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := fdk-aac

LOCAL_SRC_FILES := ../../contrib/install/lib/libfdk-aac.a
include $(PREBUILT_STATIC_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := x264

LOCAL_SRC_FILES := ../../contrib/install/lib/libx264.a
include $(PREBUILT_STATIC_LIBRARY)
###################################
include $(CLEAR_VARS)

CONTRIB_INSTALL := $(LOCAL_PATH)/../../contrib/install
PRIVATE_INCDIR := $(CONTRIB_INSTALL)/include

LOCAL_MODULE := libfqrtmpjni

LOCAL_SRC_FILES := libfqrtmpjni.c \
    native_crash_handler.c \
    libfqrtmp_events.c \
    libfqrtmp_aac.c \
    rtmp.c \
    util.c

LOCAL_C_INCLUDES := $(PRIVATE_INCDIR)
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := rtmp
LOCAL_STATIC_LIBRARIES := fdk-aac x264
include $(BUILD_SHARED_LIBRARY)
