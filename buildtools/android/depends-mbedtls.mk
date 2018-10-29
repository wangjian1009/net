LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mbedtls
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../depends/mbedtls/include
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/../../depends/mbedtls/library/*.c))

include $(BUILD_STATIC_LIBRARY)
