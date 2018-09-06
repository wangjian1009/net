set(mbedtls_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls)

file(GLOB mbedtls_source ${mbedtls_base}/library/*.c)

if (MSVC)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(mbedtls_compile_options
  -Wno-implicit-function-declaration
  )
endif ()

add_library(mbedtls STATIC ${mbedtls_source})

set_property(TARGET mbedtls PROPERTY COMPILE_OPTIONS ${mbedtls_compile_options})

set_property(TARGET mbedtls PROPERTY INCLUDE_DIRECTORIES
  ${mbedtls_base}/include
  )
