LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := wslay
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1)
LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/../../depends/wslay/include
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../depends/wslay/src/*.c)

include $(BUILD_STATIC_LIBRARY)
