ifeq ($(TARGET_ARCH),xxxxxx)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/ppp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := ip-up-ppp0.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_MODULE := ip-up-ppp0
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/ppp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := ip-down-ppp0.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_MODULE := ip-down-ppp0
include $(BUILD_EXECUTABLE)
endif
