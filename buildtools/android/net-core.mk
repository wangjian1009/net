LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := net_core
LOCAL_EXPORT_CFLAGS += $(if $(filter 0,$(APKD)),,-g)
LOCAL_CFLAGS += $(if $(filter 0,$(APKD)),,-DDEBUG=1) -Wall -Wno-unused-value -DPCRE2_CODE_UNIT_WIDTH=8
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../cpe/depends/pcre2/include \
                    $(LOCAL_PATH)/../../../cpe/include \
                    $(LOCAL_PATH)/../../core/include
LOCAL_LDLIBS := 
LOCAL_SRC_FILES += $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/../../core/src/*.c))

include $(BUILD_STATIC_LIBRARY)
