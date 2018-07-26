net_protocol_ws_base:=$(call my-dir)/../protocol_ws
net_protocol_ws_output:=$(OUTPUT_PATH)/lib/libnet_protocol_ws.a
net_protocol_ws_cpp_flags:=-I$(call my-dir)/../../cpe/include \
                           -I$(call my-dir)/../depends/wslay/include \
                           -I$(call my-dir)/../core/include \
                           -I$(net_protocol_ws_base)/include
net_protocol_ws_src:=$(wildcard $(net_protocol_ws_base)/src/*.c)
$(eval $(call def_library,net_protocol_ws))
