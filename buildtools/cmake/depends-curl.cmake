set(curl_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/curl)

file(GLOB curl_source
    ${curl_base}/lib/*.c
    ${curl_base}/lib/vauth/*.c
    ${curl_base}/lib/vtls/*.c
    )

set(curl_compile_definitions
  ${curl_compile_definitions}
  HAVE_CONFIG_H
  BUILDING_LIBCURL
  CURL_STATICLIB
  NGHTTP2_STATICLIB
  )

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(curl_compile_definitions ${curl_compile_definitions} DEBUGBUILD)
endif ()

if (MSVC)
set(curl_compile_options
  /wd4013 /wd4244
  )
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(curl_compile_options
  -Wno-unused-value
  -Wno-bitwise-op-parentheses
  )  
endif ()

add_library(curl STATIC ${curl_source})
set_property(TARGET curl PROPERTY COMPILE_OPTIONS ${curl_compile_options})
set_property(TARGET curl PROPERTY COMPILE_DEFINITIONS ${curl_compile_definitions})

set_property(TARGET curl PROPERTY INCLUDE_DIRECTORIES
  ${curl_base}/include
  ${curl_base}/lib
  ${curl_base}/../c-ares/include
  ${curl_base}/../c-ares/include/${OS_NAME}
  ${nghttp2_base}/lib/includes
  ${CMAKE_CURRENT_LIST_DIR}/../custom/nghttp2
  ${mbedtls_base}/include
  )

set(curl_depends_libraries nghttp2 c-ares mbedtls)

if (OS_NAME STREQUAL "mingw")
  set(curl_depends_libraries ${curl_depends_libraries} ws2_32 Wldap32 Iphlpapi)
endif()

target_link_libraries(curl INTERFACE ${curl_depends_libraries})
