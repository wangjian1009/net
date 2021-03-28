set(net_protocol_http_svr_base ${CMAKE_CURRENT_LIST_DIR}/../../protocol_http_svr)

file(GLOB net_protocol_http_svr_source ${net_protocol_http_svr_base}/src/*.c)

find_package(RAGEL 6.6 REQUIRED)

RAGEL_TARGET(
  net_http_svr_request_parser
  ${net_protocol_http_svr_base}/src/net_http_svr_request_parser.rl
  ${CMAKE_BINARY_DIR}/net_http_svr_request_parser.c)
set(net_protocol_http_svr_source ${net_protocol_http_svr_source} ${RAGEL_net_http_svr_request_parser_OUTPUTS})

add_library(net_protocol_http_svr STATIC ${net_protocol_http_svr_source})

set_property(TARGET net_protocol_http_svr PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_protocol_http_svr_base}/include
  ${net_protocol_http_svr_base}/src
  )

target_link_libraries(net_protocol_http_svr INTERFACE net_core)
