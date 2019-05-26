set(net_log_base ${CMAKE_CURRENT_LIST_DIR}/../../log)

file(GLOB net_log_source ${net_log_base}/src/*.c)

add_library(net_log STATIC ${net_log_source})

set_property(TARGET net_log PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls/include/mbedtls
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../driver_ev/include
  ${CMAKE_CURRENT_LIST_DIR}/../../trans/include
  ${CMAKE_CURRENT_LIST_DIR}/../../log/include
  ${net_log_base}/include
  )
