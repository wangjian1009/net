set(net_prometheus_process_base ${CMAKE_CURRENT_LIST_DIR}/../../prometheus_process)

file(GLOB net_prometheus_process_source ${net_prometheus_process_base}/src/*.c)

add_library(net_prometheus_process STATIC ${net_prometheus_process_source})

set_property(TARGET net_prometheus_process PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_prometheus_base}/include
  ${net_prometheus_process_base}/include
  )

target_link_libraries(net_prometheus_process INTERFACE net_prometheus)

