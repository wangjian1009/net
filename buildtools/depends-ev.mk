ev_base:=$(call my-dir)/../depends/libev
ev_output:=$(OUTPUT_PATH)/lib/libev.a
ev_cpp_flags:=-I$(ev_base)/include -I$(ev_base)/src/$(OS_NAME)
ev_c_flags:=-Wno-unused-value -Wno-bitwise-op-parentheses
ev_src:=$(ev_base)/src/ev.c
$(eval $(call def_library,ev))
