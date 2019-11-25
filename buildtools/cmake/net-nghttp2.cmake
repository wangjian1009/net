set(net_nghttp2_base ${CMAKE_CURRENT_LIST_DIR}/../../nghttp2)

file(GLOB net_nghttp2_source ${net_nghttp2_base}/src/*.c)

add_library(net_nghttp2 STATIC ${net_nghttp2_source})

set_property(TARGET net_nghttp2 PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${nghttp2_base}/lib/includes
  ${CMAKE_CURRENT_LIST_DIR}/../custom/nghttp2
  ${net_nghttp2_base}/include
  )
