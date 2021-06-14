set(net_driver_dq_base ${CMAKE_CURRENT_LIST_DIR}/../../driver_dq)

file(GLOB net_driver_dq_source ${net_driver_dq_base}/src/*.m)

add_library(net_driver_dq STATIC ${net_driver_dq_source})

set_property(TARGET net_driver_dq PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../driver_sock/include
  ${net_driver_dq_base}/include
  )

target_link_libraries(net_driver_dq INTERFACE net_driver_sock)
