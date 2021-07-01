set(mbedtls_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/mbedtls)

file(GLOB mbedtls_source ${mbedtls_base}/library/*.c)

if(WIN32)
  set(mbedtls_link_libraries Ws2_32)
endif()

set(mbedtls_compile_definitions
  MBEDTLS_THREADING_PTHREAD
  MBEDTLS_THREADING_C)

if (MSVC)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(mbedtls_compile_options
  -Wno-implicit-function-declaration
  )
endif ()
  
add_library(mbedtls STATIC ${mbedtls_source})
target_compile_options(mbedtls PRIVATE ${mbedtls_compile_options})
target_include_directories(mbedtls PUBLIC ${mbedtls_base}/include)
target_compile_definitions(mbedtls INTERFACE ${mbedtls_compile_definitions})

target_link_libraries(mbedtls INTERFACE ${mbedtls_link_libraries})
