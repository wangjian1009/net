set(net_prometheus_base ${CMAKE_CURRENT_LIST_DIR}/../../prometheus)

file(GLOB net_prometheus_source ${net_prometheus_base}/src/*.c)

add_library(net_prometheus STATIC ${net_prometheus_source})

set_property(TARGET net_prometheus PROPERTY INCLUDE_DIRECTORIES
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${pcre2_base}/include
  ${net_prometheus_base}/include
  )

set_property(TARGET net_prometheus PROPERTY COMPILE_DEFINITIONS
  PCRE2_STATIC
  PCRE2_CODE_UNIT_WIDTH=8
  )

target_link_libraries(net_prometheus INTERFACE cpe_utils pcre2)
