set(net_driver_http2_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_http2)

file(GLOB net_driver_http2_source ${net_driver_http2_base}/src/*.c)

set(net_driver_http2_compile_definitions NGHTTP2_STATICLIB)

add_library(net_driver_http2 STATIC ${net_driver_http2_source})
set_property(TARGET net_driver_http2 PROPERTY COMPILE_DEFINITIONS ${net_driver_http2_compile_definitions})

set_property(TARGET net_driver_http2 PROPERTY INCLUDE_DIRECTORIES
  ${nghttp2_base}/lib/includes
  ${nghttp2_custom}
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${net_core_base}/include
  ${net_driver_http2_base}/include
  )

target_link_libraries(net_driver_http2 INTERFACE net_core nghttp2)
