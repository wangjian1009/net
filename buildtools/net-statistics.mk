net_statistics_base:=$(call my-dir)/../statistics
net_statistics_output:=$(OUTPUT_PATH)/lib/libnet_statistics.a
net_statistics_cpp_flags:=-I$(call my-dir)/../../cpe/include \
                          -I$(net_statistics_base)/include
net_statistics_src:=$(wildcard $(net_statistics_base)/src/*.c)
$(eval $(call def_library,net_statistics))
