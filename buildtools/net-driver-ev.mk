net_driver_ev_base:=$(call my-dir)/../driver_ev
net_driver_ev_output:=$(OUTPUT_PATH)/lib/libnet_driver_ev.a
net_driver_ev_cpp_flags:=-I$(call my-dir)/../../cpe/include \
                         -I$(call my-dir)/../depends/libev/include \
                         -I$(call my-dir)/../core/include \
                         -I$(call my-dir)/../driver_sock/include \
                         -I$(net_driver_ev_base)/include
net_driver_ev_src:=$(wildcard $(net_driver_ev_base)/src/*.c)
$(eval $(call def_library,net_driver_ev))
