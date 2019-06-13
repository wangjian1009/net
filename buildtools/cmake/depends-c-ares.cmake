set(c_ares_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/c-ares)

file(GLOB c_ares_source
    ${c_ares_base}/src/ares_*.c
    )

set(c_ares_source
    ${c_ares_source}
    ${c_ares_base}/src/bitncmp.c
    ${c_ares_base}/src/inet_net_pton.c
    ${c_ares_base}/src/inet_ntop.c
    )

set(c_ares_compile_definitions
  ${c_ares_compile_definitions}
  HAVE_CONFIG_H
  )

if (MSVC)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(c_ares_compile_options
  -Wno-implicit-function-declaration
  )
endif ()

add_library(c-ares STATIC ${c_ares_source})
set_property(TARGET c-ares PROPERTY COMPILE_OPTIONS ${c_ares_compile_options})
set_property(TARGET c-ares PROPERTY COMPILE_DEFINITIONS ${c_ares_compile_definitions})

set_property(TARGET c-ares PROPERTY INCLUDE_DIRECTORIES
  ${c_ares_base}/include
  ${c_ares_base}/include/${OS_NAME}
  ${c_ares_base}/src/${OS_NAME}
  )
