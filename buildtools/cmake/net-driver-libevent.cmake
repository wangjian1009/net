set(net_driver_libevent_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_libevent)

file(GLOB net_driver_libevent_source ${net_driver_libevent_base}/src/*.c)

add_library(net_driver_libevent STATIC ${net_driver_libevent_source})

set_property(TARGET net_driver_libevent PROPERTY INCLUDE_DIRECTORIES
  ${libevent_base}/include
  ${libevent_custom}
  ${CMAKE_CURRENT_LIST_DIR}/../../driver_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_driver_libevent_base}/include
  )

add_dependencies(net_driver_libevent libevent)
