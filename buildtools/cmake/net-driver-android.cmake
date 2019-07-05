set(net_driver_android_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_android)

file(GLOB net_driver_android_source ${net_driver_android_base}/src/*.c)

add_library(net_driver_android STATIC ${net_driver_android_source})

set_property(TARGET net_driver_android PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev/include
  ${CMAKE_CURRENT_LIST_DIR}/../../driver_sock/include
  ${net_driver_android_base}/include
  )
