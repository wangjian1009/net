set(wslay_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/wslay)

file(GLOB wslay_source ${wslay_base}/src/*.c)

if (MSVC)
set(wslay_compile_definitions
  ${wslay_compile_definitions}
  ssize_t=int
  HAVE_WINSOCK2_H
  )
set(wslay_compile_options
  /wd4013 /wd4244
  )
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(wslay_compile_options
  -Wno-unused-value
  -Wno-bitwise-op-parentheses
  )  
endif ()

if (OS_NAME MATCHES "cygwin")
set(wslay_compile_definitions
  ${wslay_compile_definitions}
  HAVE_NETINET_IN_H
  )
endif ()
  
add_library(wslay STATIC ${wslay_source})
set_property(TARGET wslay PROPERTY COMPILE_OPTIONS ${wslay_compile_options})
set_property(TARGET wslay PROPERTY COMPILE_DEFINITIONS ${wslay_compile_definitions})

set_property(TARGET wslay PROPERTY INCLUDE_DIRECTORIES
  ${wslay_base}/include
  ${wslay_base}/src/${OS_NAME}
  )
