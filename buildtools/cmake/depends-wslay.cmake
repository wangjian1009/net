set(wslay_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/wslay)

file(GLOB wslay_source ${wslay_base}/src/*.c)

add_library(wslay STATIC ${wslay_source})

if (GCC)
set_property(TARGET wslay PROPERTY COMPILE_OPTIONS
  -Wno-unused-value
  -Wno-bitwise-op-parentheses
  )
endif ()

if (MSVC)
set_property(TARGET wslay PROPERTY COMPILE_DEFINITIONS
  ssize_t=int
  )
endif ()

set_property(TARGET wslay PROPERTY INCLUDE_DIRECTORIES
  ${wslay_base}/include
  ${wslay_base}/src/${OS_NAME}
  )
