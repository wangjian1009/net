#tool_ndt7
if (TOOL_NDT_OUTPUT_PATH)
  set(tool_ndt7_output ${TOOL_NDT_OUTPUT_PATH})
else()
  set(tool_ndt7_output ${CMAKE_BINARY_DIR})
endif()

message(STATUS "tool_ndt7_output=${tool_ndt7_output}")

file(GLOB tool_ndt7_source ${net_ndt7_base}/cmd/*.c)

set(tool_ndt7_includes
    ${argtable2_base}/include
    ${ev_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_sock_base}/include
    ${net_core_base}/include
    ${net_dns_base}/include
    ${net_dns_udns_base}/include
    ${net_driver_ev_base}/include
    ${net_protocol_http_base}/include
    ${net_driver_ssl_base}/include
    ${net_ndt7_base}/include
    )

set(tool_ndt7_link_libraries
  cpe_utils net_dns net_dns_udns net_driver_ssl net_driver_ev net_protocol_http argtable2)

add_executable(tool_ndt7 ${tool_ndt7_source})
set_property(TARGET tool_ndt7 PROPERTY INCLUDE_DIRECTORIES ${tool_ndt7_includes})
set_property(TARGET tool_ndt7 PROPERTY LINK_LIBRARIES ${tool_ndt7_link_libraries})
set_property(TARGET tool_ndt7 PROPERTY RUNTIME_OUTPUT_DIRECTORY ${tool_ndt7_output})
