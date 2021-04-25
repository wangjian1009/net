set(tool_url_base ${CMAKE_CURRENT_LIST_DIR}/../../tool_url)

#tool_url
if (TOOL_URL_OUTPUT_PATH)
  set(tool_url_output ${TOOL_URL_OUTPUT_PATH})
else()
  set(tool_url_output ${CMAKE_BINARY_DIR})
endif()

message(STATUS "tool_url_output=${tool_url_output}")

file(GLOB tool_url_source ${tool_url_base}/*.c)

set(tool_url_includes
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
    )

set(tool_url_link_libraries
  cpe_utils net_trans net_dns net_dns_udns net_driver_ssl net_driver_ev net_protocol_http argtable2)

add_executable(tool_url ${tool_url_source})
set_property(TARGET tool_url PROPERTY INCLUDE_DIRECTORIES ${tool_url_includes})
set_property(TARGET tool_url PROPERTY LINK_LIBRARIES ${tool_url_link_libraries})
set_property(TARGET tool_url PROPERTY RUNTIME_OUTPUT_DIRECTORY ${tool_url_output})
