set(net_trans_base ${CMAKE_CURRENT_LIST_DIR}/../../trans)

file(GLOB net_trans_source ${net_trans_base}/src/*.c)

add_library(net_trans STATIC ${net_trans_source})

set(net_trans_compile_definitions
  CURL_STATICLIB
  )

set_property(TARGET net_trans PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/curl/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../protocol_http/include
  ${net_trans_base}/include
  )

set_property(TARGET net_trans PROPERTY COMPILE_DEFINITIONS ${net_trans_compile_definitions})

set(net_trans_libraries
  cpe_utils net_core curl)

if (OS_NAME MATCHES "linux")
  set(net_trans_libraries ${net_trans_libraries} z)
elseif (OS_NAME MATCHES "mac")
  set(net_trans_libraries ${net_trans_libraries} z)
elseif (OS_NAME MATCHES "android")
  set(net_trans_libraries ${net_trans_libraries} z)
elseif (OS_NAME MATCHES "vc")
elseif (OS_NAME MATCHES "mingw")
  set(net_trans_libraries ${net_trans_libraries} z)
endif()

target_link_libraries(net_trans INTERFACE ${net_trans_libraries})
