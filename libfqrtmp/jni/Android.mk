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

LOCAL_MODULE := anw.10

LOCAL_SRC_FILES := ../../contrib/vlclibs/libanw.10.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := anw.13

LOCAL_SRC_FILES := ../../contrib/vlclibs/libanw.13.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := anw.14

LOCAL_SRC_FILES := ../../contrib/vlclibs/libanw.14.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := anw.18

LOCAL_SRC_FILES := ../../contrib/vlclibs/libanw.18.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := anw.21

LOCAL_SRC_FILES := ../../contrib/vlclibs/libanw.21.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := iomx.10

LOCAL_SRC_FILES := ../../contrib/vlclibs/libiomx.10.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := iomx.13

LOCAL_SRC_FILES := ../../contrib/vlclibs/libiomx.13.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := iomx.14

LOCAL_SRC_FILES := ../../contrib/vlclibs/libiomx.14.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := vlcjni

LOCAL_SRC_FILES := ../../contrib/vlclibs/libvlcjni.so
include $(PREBUILT_SHARED_LIBRARY)
###################################
include $(CLEAR_VARS)

CONTRIB_INSTALL := $(LOCAL_PATH)/../../contrib/install
PRIVATE_INCDIR := $(CONTRIB_INSTALL)/include

LOCAL_MODULE := libfqrtmpjni

LOCAL_SRC_FILES := libfqrtmpjni.cpp \
    native_crash_handler.cpp \
    libfqrtmp_events.cpp \
    audio_encoder.cpp \
    video_encoder.cpp \
    rtmp_handler.cpp \
    raw_parser.cpp \
    common.cpp \
    jitter_buffer.cpp \
    xutil/xfile.cpp \
    xutil/xutil.cpp \
    xutil/xmedia.cpp

LOCAL_C_INCLUDES := $(PRIVATE_INCDIR) $(LOCAL_PATH)/xutil
LOCAL_CFLAGS := -Wall
LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := rtmp
LOCAL_STATIC_LIBRARIES := fdk-aac x264 libyuv_static
include $(BUILD_SHARED_LIBRARY)
####################################
include $(call all-makefiles-under,$(LOCAL_PATH))
