set(mbedtls_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls)

file(GLOB mbedtls_source ${mbedtls_base}/library/*.c)

add_library(mbedtls STATIC ${mbedtls_source})

if (GCC)
set_property(TARGET mbedtls PROPERTY COMPILE_OPTIONS
  -Wno-implicit-function-declaration
  )
endif ()

set_property(TARGET mbedtls PROPERTY INCLUDE_DIRECTORIES
  ${mbedtls_base}/include
  )
