net_trans_base:=$(call my-dir)/../trans
net_trans_output:=$(OUTPUT_PATH)/lib/libnet_trans.a
net_trans_cpp_flags:=-I$(net_trans_base)/../../cpe/include \
                     -I$(net_trans_base)/../core/include \
                     -I$(net_trans_base)/../protocol_http/include \
                     -I$(net_trans_base)/include
net_trans_src:=$(wildcard $(net_trans_base)/src/*.c)
$(eval $(call def_library,net_trans))
