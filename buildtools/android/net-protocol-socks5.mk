LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := net_protocol_socks5
LOCAL_EXPORT_CFLAGS += $(if $(filter 0,$(APKD)),,-g)
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1) -Wall -Wno-unused-value
LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/../../../cpe/include \
                     $(LOCAL_PATH)/../../core/include \
                     $(LOCAL_PATH)/../../protocol_socks5/include
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(wildcard $(LOCAL_PATH)/../../protocol_socks5/src/*.c)

include $(BUILD_STATIC_LIBRARY)
