set(ev_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev)

set(ev_source
  ${ev_base}/src/ev.c
  )

if (MSVC)
  set(ev_compile_options /wd4244)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
  set(ev_compile_options
    -Wno-unused-value
    -Wno-bitwise-op-parentheses
    )
endif ()

add_library(ev STATIC ${ev_source})

set_property(TARGET ev PROPERTY INCLUDE_DIRECTORIES
  ${ev_base}/include
  ${ev_base}/src/${OS_NAME}
  )

set_property(TARGET ev PROPERTY COMPILE_OPTIONS ${ev_compile_options})
