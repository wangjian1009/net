set(net_core_base ${CMAKE_CURRENT_LIST_DIR}/../../core)

file(GLOB net_core_source ${net_core_base}/src/*.c)

set(net_core_includes
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_sock_base}/include
  ${pcre2_base}/include
  ${net_core_base}/include
  )

add_library(net_core STATIC ${net_core_source})

set_property(TARGET net_core PROPERTY INCLUDE_DIRECTORIES ${net_core_includes})

set_property(TARGET net_core PROPERTY COMPILE_DEFINITIONS
  PCRE2_STATIC
  PCRE2_CODE_UNIT_WIDTH=8
  )

target_link_libraries(net_core INTERFACE cpe_utils_sock pcre2)
