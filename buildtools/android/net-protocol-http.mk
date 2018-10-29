LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := net_protocol_http
LOCAL_EXPORT_CFLAGS += $(if $(filter 0,$(APKD)),,-g)
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1) -Wall
LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/../../depends/mbedtls/include \
                     $(LOCAL_PATH)/../../../cpe/include \
                     $(LOCAL_PATH)/../../core/include \
                     $(LOCAL_PATH)/../../protocol_http/include
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/../../protocol_http/src/*.c))

include $(BUILD_STATIC_LIBRARY)
