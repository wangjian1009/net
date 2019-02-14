curl_base:=$(call my-dir)/../depends/curl
curl_output:=$(OUTPUT_PATH)/lib/libcurl.a
curl_cpp_flags:=-I$(curl_base)/include -I$(curl_base)/include/$(OS_NAME)/curl -I$(curl_base)/lib/$(OS_NAME) \
                -I$(call my-dir)/../depends/mbedtls/include -I$(call my-dir)/../depends/mbedtls/include/mbedtls \
                -DHAVE_CONFIG_H
curl_c_flags:=-Wno-deprecated-declarations
curl_src:=$(wildcard $(curl_base)/lib/*.c)
$(eval $(call def_library,curl))
