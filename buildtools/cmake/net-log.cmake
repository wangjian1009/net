set(net_log_base ${CMAKE_CURRENT_LIST_DIR}/../../log)

file(GLOB net_log_source ${net_log_base}/src/*.c)

add_library(net_log STATIC ${net_log_source})

set(net_log_includes
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls/include/mbedtls
  ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/fsm/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/vfs/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${CMAKE_CURRENT_LIST_DIR}/../../driver_ev/include
  ${CMAKE_CURRENT_LIST_DIR}/../../trans/include
  ${net_log_base}/include
  )

if (MSVC)

set(net_log_includes
    ${net_log_includes}
    ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/depends/pthread/vc/include
    )
    
endif ()

set_property(TARGET net_log PROPERTY INCLUDE_DIRECTORIES ${net_log_includes})
