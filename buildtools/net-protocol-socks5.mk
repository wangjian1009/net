net_protocol_socks5_base:=$(call my-dir)/../protocol_socks5
net_protocol_socks5_output:=$(OUTPUT_PATH)/lib/libnet_protocol_socks5.a
net_protocol_socks5_cpp_flags:=-I$(net_protocol_socks5_base)/../../cpe/include \
                               -I$(net_protocol_socks5_base)/../core/include \
                               -I$(net_protocol_socks5_base)/include
net_protocol_socks5_src:=$(wildcard $(net_protocol_socks5_base)/src/*.c)
$(eval $(call def_library,net_protocol_socks5))
