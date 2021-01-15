set(net_driver_ssl_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_ssl)

file(GLOB net_driver_ssl_source ${net_driver_ssl_base}/src/*.c)

add_library(net_driver_ssl STATIC ${net_driver_ssl_source})

set_property(TARGET net_driver_ssl PROPERTY INCLUDE_DIRECTORIES
  ${openssl_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_driver_ssl_base}/include
  )
