net_dns_base:=$(call my-dir)/../dns
net_dns_output:=$(OUTPUT_PATH)/lib/libnet_dns.a
net_dns_cpp_flags:=-I$(net_dns_base)/../../cpe/include \
                   -I$(net_dns_base)/../core/include \
                   -I$(net_dns_base)/include
net_dns_src:=$(wildcard $(net_dns_base)/src/*.c)
$(eval $(call def_library,net_dns))
