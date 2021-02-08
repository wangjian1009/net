set(kcp_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/kcp)

file(GLOB kcp_source ${kcp_base}/*.c)

add_library(kcp STATIC ${kcp_source})
set_property(TARGET kcp PROPERTY COMPILE_OPTIONS ${kcp_compile_options})
set_property(TARGET kcp PROPERTY COMPILE_DEFINITIONS ${kcp_compile_definitions})
set_property(TARGET kcp PROPERTY INCLUDE_DIRECTORIES ${kcp_base})
