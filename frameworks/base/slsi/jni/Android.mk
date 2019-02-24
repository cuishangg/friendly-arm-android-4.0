LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_slsi_sec_android_HdmiService.cpp \
    onload.cpp

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE)

LOCAL_SHARED_LIBRARIES := \
						  libcutils \
						  libandroid_runtime \
						  libnativehelper \
						  libbinder \
						  libutils 

ifeq ($(S5P_BOARD_USES_HDMI),true)
LOCAL_C_INCLUDES += \
					device/friendly-arm/mini210/include

LOCAL_SHARED_LIBRARIES += libTVOut

LOCAL_CFLAGS     += -DS5P_BOARD_USES_HDMI
endif

LOCAL_PRELINK_MODULE := false

ifeq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_OS),linux)
ifeq ($(TARGET_ARCH),x86)
LOCAL_LDLIBS += -lpthread -ldl -lrt
endif
endif
endif

ifeq ($(WITH_MALLOC_LEAK_CHECK),true)
	LOCAL_CFLAGS += -DMALLOC_LEAK_CHECK
endif

LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE:= libhdmiservice_jni

include $(BUILD_SHARED_LIBRARY)

