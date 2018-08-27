net_protocol_http_base:=$(call my-dir)/../protocol_http
net_protocol_http_output:=$(OUTPUT_PATH)/lib/libnet_protocol_http.a
net_protocol_http_cpp_flags:=-I$(call my-dir)/../../cpe/include \
                           -I$(call my-dir)/../depends/httplay/include \
                           -I$(call my-dir)/../core/include \
                           -I$(net_protocol_http_base)/include
net_protocol_http_src:=$(wildcard $(net_protocol_http_base)/src/*.c)
$(eval $(call def_library,net_protocol_http))
