set(udns_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/udns)

file(GLOB udns_source ${udns_base}/*.c)

add_library(udns STATIC ${udns_source})
set_property(TARGET udns PROPERTY COMPILE_OPTIONS ${udns_compile_options})
set_property(TARGET udns PROPERTY COMPILE_DEFINITIONS ${udns_compile_definitions})

set_property(TARGET udns PROPERTY INCLUDE_DIRECTORIES
  ${udns_base}
  )
