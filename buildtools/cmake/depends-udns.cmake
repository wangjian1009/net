set(udns_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/udns)

file(GLOB udns_source
  ${udns_base}/udns_dn.c
  ${udns_base}/udns_dntosp.c
  ${udns_base}/udns_parse.c
  ${udns_base}/udns_resolver.c
  ${udns_base}/udns_init.c
  ${udns_base}/udns_misc.c
  ${udns_base}/udns_XtoX.c
  ${udns_base}/udns_rr_a.c
  ${udns_base}/udns_rr_ptr.c
  ${udns_base}/udns_rr_mx.c
  ${udns_base}/udns_rr_txt.c
  ${udns_base}/udns_bl.c
  ${udns_base}/udns_rr_srv.c
  ${udns_base}/udns_rr_naptr.c
  ${udns_base}/udns_codes.c
  ${udns_base}/udns_jran.c
)

if (MSVC)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_IS_GNUCC)
set(udns_compile_options
  -Wno-enum-conversion
  )  
endif ()

if (OS_NAME STREQUAL mingw)
  set(udns_compile_definitions WINDOWS)
else()
  set(udns_compile_definitions HAVE_INET_PTON_NTOP)
endif()

add_library(udns STATIC ${udns_source})
set_property(TARGET udns PROPERTY COMPILE_OPTIONS ${udns_compile_options})
set_property(TARGET udns PROPERTY COMPILE_DEFINITIONS ${udns_compile_definitions})

set_property(TARGET udns PROPERTY INCLUDE_DIRECTORIES
  ${udns_base}
  )
