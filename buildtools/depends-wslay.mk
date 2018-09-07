wslay_base:=$(call my-dir)/../depends/wslay
wslay_output:=$(OUTPUT_PATH)/lib/libwslay.a
wslay_cpp_flags:=-I$(wslay_base)/include -I$(wslay_base)/src/$(OS_NAME)
wslay_c_flags:=-Wno-unused-value -Wno-bitwise-op-parentheses 
wslay_src:=$(wildcard $(wslay_base)/src/*.c)
$(eval $(call def_library,wslay))
