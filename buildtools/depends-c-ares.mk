c-ares_base:=$(call my-dir)/../depends/c-ares
c-ares_output:=$(OUTPUT_PATH)/lib/libc-ares.a
c-ares_cpp_flags:=-I$(c-ares_base)/include \
                  -I$(c-ares_base)/include/$(OS_NAME) \
                  -I$(c-ares_base)/src/$(OS_NAME) \
                  -DHAVE_CONFIG_H
c-ares_c_flags:=-Wno-unused-value -Wno-bitwise-op-parentheses 
c-ares_src:=$(filter-out %/acountry.c %/adig.c %/ahost.c, $(wildcard $(c-ares_base)/src/*.c))
$(eval $(call def_library,c-ares))
