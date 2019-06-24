set(net_core_base ${CMAKE_CURRENT_LIST_DIR}/../../core)

file(GLOB net_core_source ${net_core_base}/src/*.c)

add_library(net_core STATIC ${net_core_source})

set_property(TARGET net_core PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils_sock/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/depends/pcre2/include
  ${net_core_base}/include
  )

set_property(TARGET net_core PROPERTY COMPILE_DEFINITIONS
  PCRE2_STATIC
  PCRE2_CODE_UNIT_WIDTH=8
  )

