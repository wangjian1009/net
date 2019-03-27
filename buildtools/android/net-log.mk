LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := net_log
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1)
LOCAL_CFLAGS += 
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../depends/curl/include \
                    $(LOCAL_PATH)/../../depends/mbedtls/include/mbedtls \
                    $(LOCAL_PATH)/../../log/src
LOCAL_SRC_FILES +=  $(patsubst $(LOCAL_PATH)/%,%,\
                        $(wildcard $(LOCAL_PATH)/../../log/src/*.c))

include $(BUILD_STATIC_LIBRARY)
