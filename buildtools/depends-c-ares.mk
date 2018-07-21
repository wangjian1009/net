c_ares_base:=$(call my-dir)/../depends/c-ares
c_ares_output:=$(OUTPUT_PATH)/lib/libc-ares.a
c_ares_cpp_flags:=-DHAVE_CONFIG_H \
                  -I$(c_ares_base)/include \
                  -I$(c_ares_base)/include/$(OS_NAME) \
                  -I$(c_ares_base)/src/$(OS_NAME)
c_ares_src:=$(filter-out %/acountry.c %/adig.c %/ahost.c, $(wildcard $(c_ares_base)/src/*.c))
$(eval $(call def_library,c_ares))
