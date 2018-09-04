set(ev_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev)

set(ev_source
  ${ev_base}/src/ev.c
  )

add_library(ev STATIC ${ev_source})

set_property(TARGET ev PROPERTY INCLUDE_DIRECTORIES
  ${ev_base}/include
  ${ev_base}/src/${OS_NAME}
  )

set_property(TARGET ev PROPERTY COMPILE_OPTIONS
  -Wno-unused-value
  -Wno-bitwise-op-parentheses
  )
