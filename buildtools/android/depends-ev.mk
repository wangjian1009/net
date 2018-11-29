LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ev
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1)
LOCAL_CFLAGS +=  -Wno-unused -Wno-parentheses -Wno-unused-variable
LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/../../depends/libev/include $(LOCAL_PATH)/../../depends/libev/src/android
LOCAL_SRC_FILES += ../../depends/libev/src/ev.c

include $(BUILD_STATIC_LIBRARY)
