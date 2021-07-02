set(ev_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/libev)

set(ev_source
  ${ev_base}/src/ev.c
  )

if (MSVC)
  set(ev_compile_options /wd4244)

  set(ev_compile_definitions
    FD_SETSIZE=2048
    )

elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
  set(ev_compile_options
    -Wno-unused-value
    -Wno-bitwise-op-parentheses
    )
endif ()

add_library(ev STATIC ${ev_source})

if(WIN32)
  list(APPEND ev_link_libraries Ws2_32)
endif()

set_property(TARGET ev PROPERTY INCLUDE_DIRECTORIES
  ${ev_base}/include
  ${ev_base}/src/${OS_NAME}
  )

set_property(TARGET ev PROPERTY COMPILE_OPTIONS ${ev_compile_options})
set_property(TARGET ev PROPERTY COMPILE_DEFINITIONS ${ev_compile_definitions})
target_link_libraries(ev INTERFACE ${ev_link_libraries})
