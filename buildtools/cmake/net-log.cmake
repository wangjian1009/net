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
  ${CMAKE_CURRENT_LIST_DIR}/../../trans/include
  ${net_log_base}/include
  )

if (OS_NAME STREQUAL "android")
  set(net_log_compile_definitions ${net_log_compile_options} NET_LOG_MULTI_THREAD)
endif()

if (MSVC)
  set(net_log_includes
    ${net_log_includes}
    ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/depends/pthread/vc/include
    )
endif ()

set_property(TARGET net_log PROPERTY INCLUDE_DIRECTORIES ${net_log_includes})
set_property(TARGET net_log PROPERTY COMPILE_OPTIONS ${net_log_compile_options})
set_property(TARGET net_log PROPERTY COMPILE_DEFINITIONS ${net_log_compile_definitions})

target_link_libraries(net_log INTERFACE cpe_fsm net_core)
