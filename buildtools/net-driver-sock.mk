net_driver_sock_base:=$(call my-dir)/../driver_sock
net_driver_sock_output:=$(OUTPUT_PATH)/lib/libnet_driver_sock.a
net_driver_sock_cpp_flags:=-I$(call my-dir)/../../cpe/include \
                           -I$(call my-dir)/../core/include \
                           -I$(net_driver_sock_base)/include
net_driver_sock_src:=$(wildcard $(net_driver_sock_base)/src/*.c)
$(eval $(call def_library,net_driver_sock))
