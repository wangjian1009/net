net_router_base:=$(call my-dir)/../router
net_router_output:=$(OUTPUT_PATH)/lib/libnet_router.a
net_router_cpp_flags:=-I$(net_router_base)/../../cpe/include \
                      -I$(net_router_base)/../core/include \
                      -I$(net_router_base)/include
net_router_src:=$(wildcard $(net_router_base)/src/*.c)
$(eval $(call def_library,net_router))
