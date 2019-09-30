set(net_utils_crypto_base ${CMAKE_CURRENT_LIST_DIR}/../../utils_crypto)

file(GLOB net_utils_crypto_source ${net_utils_crypto_base}/src/*.c)

add_library(net_utils_crypto STATIC ${net_utils_crypto_source})

set_property(TARGET net_utils_crypto PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls/include/mbedtls
  ${net_utils_crypto_base}/include
  )

set_property(TARGET net_utils_crypto PROPERTY COMPILE_DEFINITIONS ${net_utils_crypto_compile_definitions})
