curl_base:=$(call my-dir)/../depends/curl
curl_output:=$(OUTPUT_PATH)/lib/libcurl.a
curl_cpp_flags:=-I$(curl_base)/include \
                -I$(curl_base)/lib \
                -I$(call my-dir)/../depends/c-ares/include \
                -I$(call my-dir)/../depends/c-ares/include/$(OS_NAME) \
                -I$(call my-dir)/../depends/mbedtls/include \
                -I$(call my-dir)/../depends/mbedtls/include/mbedtls \
                -DHAVE_CONFIG_H \
                -DBUILDING_LIBCURL

curl_c_flags:=-Wno-deprecated-declarations
curl_src:=$(wildcard $(curl_base)/lib/*.c) \
          $(wildcard $(curl_base)/lib/vauth/*.c) \
          $(wildcard $(curl_base)/lib/vtls/*.c)

$(eval $(call def_library,curl))
