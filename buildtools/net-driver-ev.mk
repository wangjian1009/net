net_driver_ev_base:=$(call my-dir)/../driver_ev
net_driver_ev_output:=$(OUTPUT_PATH)/lib/libnet_driver_ev.a
net_driver_ev_cpp_flags:=-I$(net_driver_ev_base)/../../cpe/include \
                             -I$(net_driver_ev_base)/../depends/libev/include \
                             -I$(net_driver_ev_base)/../core/include \
                             -I$(net_driver_ev_base)/include
net_driver_ev_src:=$(wildcard $(net_driver_ev_base)/src/*.c)
$(eval $(call def_library,net_driver_ev))
