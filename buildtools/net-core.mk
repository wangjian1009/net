net_core_base:=$(call my-dir)/../core
net_core_output:=$(OUTPUT_PATH)/lib/libnet_core.a
net_core_cpp_flags:=-DPCRE2_CODE_UNIT_WIDTH=8 \
                    -I$(net_core_base)/../../cpe/include \
                    -I$(net_core_base)/../../cpe/depends/pcre2/include \
                    -I$(net_core_base)/include
net_core_src:=$(wildcard $(net_core_base)/src/*.c)
$(eval $(call def_library,net_core))
