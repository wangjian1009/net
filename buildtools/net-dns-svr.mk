net_dns_svr_base:=$(call my-dir)/../dns_svr
net_dns_svr_output:=$(OUTPUT_PATH)/lib/libnet_dns_svr.a
net_dns_svr_cpp_flags:=-I$(net_dns_svr_base)/../../cpe/include \
                   -I$(net_dns_svr_base)/../core/include \
                   -I$(net_dns_svr_base)/include
net_dns_svr_src:=$(wildcard $(net_dns_svr_base)/src/*.c)
$(eval $(call def_library,net_dns_svr))
