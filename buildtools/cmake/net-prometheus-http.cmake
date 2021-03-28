set(net_prometheus_http_base ${CMAKE_CURRENT_LIST_DIR}/../../prometheus_http)

file(GLOB net_prometheus_http_source ${net_prometheus_http_base}/src/*.c)

add_library(net_prometheus_http STATIC ${net_prometheus_http_source})

set_property(TARGET net_prometheus_http PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_protocol_http_svr_base}/include
  ${net_prometheus_base}/include
  ${net_prometheus_http_base}/include
  )

target_link_libraries(net_prometheus_http INTERFACE net_prometheus net_protocol_http_svr)

