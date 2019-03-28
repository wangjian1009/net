net_log_base:=$(call my-dir)/../log
net_log_output:=$(OUTPUT_PATH)/lib/libnet_log.a
net_log_cpp_flags:=-I$(call my-dir)/../depends/curl/include \
                   -I$(call my-dir)/../depends/mbedtls/include/mbedtls \
                   -I$(call my-dir)/../depends/libev/include \
                   -I$(net_log_base)/../../cpe/include \
                   -I$(net_log_base)/../core/include \
                   -I$(net_log_base)/../driver_ev/include \
                   -I$(net_log_base)/../log/include \
                   -I$(net_log_base)
net_log_c_flags:=-Wno-implicit-function-declaration
net_log_src:=$(wildcard $(net_log_base)/src/*.c)
$(eval $(call def_library,net_log))
