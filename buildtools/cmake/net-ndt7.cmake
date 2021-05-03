set(net_ndt7_base ${CMAKE_CURRENT_LIST_DIR}/../../ndt7)

file(GLOB net_ndt7_source ${net_ndt7_base}/src/*.c)

add_library(net_ndt7 STATIC ${net_ndt7_source})

set_property(TARGET net_ndt7 PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${yajl_base}/include
  ${net_core_base}/include
  ${net_protocol_http_base}/include
  ${net_driver_ssl_base}/include
  ${net_ndt7_base}/include
  )

target_link_libraries(net_ndt7 INTERFACE net_core net_driver_ssl net_protocol_http yajl)
