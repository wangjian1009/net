curl_base:=$(call my-dir)/../depends/curl
curl_output:=$(OUTPUT_PATH)/lib/libcurl.a
curl_cpp_flags:=-DHAVE_CONFIG_H \
                -I$(curl_base)/../c-ares/include \
                -I$(curl_base)/../c-ares/include/$(OS_NAME) \
                -I$(curl_base)/../openssl/include  \
                -I$(curl_base)/../openssl/include/$(OS_NAME) \
                -I$(curl_base)/include \
                -I$(curl_base)/include/$(OS_NAME)/curl \
                -I$(curl_base)/lib \
                -I$(curl_base)/lib/$(OS_NAME)
curl_src:=$(wildcard $(curl_base)/lib/*.c)
$(eval $(call def_library,curl))
