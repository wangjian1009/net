net_protocol_proxy_http_base:=$(call my-dir)/../protocol_proxy_http
net_protocol_proxy_http_output:=$(OUTPUT_PATH)/lib/libnet_protocol_proxy_http.a
net_protocol_proxy_http_cpp_flags:=-I$(net_protocol_proxy_http_base)/../../cpe/include \
                                   -I$(net_protocol_proxy_http_base)/../core/include \
                                   -I$(net_protocol_proxy_http_base)/include
net_protocol_proxy_http_src:=$(wildcard $(net_protocol_proxy_http_base)/src/*.c)
$(eval $(call def_library,net_protocol_proxy_http))
