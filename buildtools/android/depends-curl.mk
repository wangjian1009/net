LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := curl
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../depends/curl/include \
                    $(LOCAL_PATH)/../../depends/curl/lib \
                    $(LOCAL_PATH)/../../depends/mbedtls/include \
                    $(LOCAL_PATH)/../../depends/mbedtls/include/mbedtls
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%, \
                       $(wildcard $(LOCAL_PATH)/../../depends/curl/lib/*.c) \
                       $(wildcard $(LOCAL_PATH)/../../depends/curl/lib/vauth/*.c) \
                       $(wildcard $(LOCAL_PATH)/../../depends/curl/lib/vtls/*.c) \
                   )

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DBUILDING_LIBCURL

include $(BUILD_STATIC_LIBRARY)
