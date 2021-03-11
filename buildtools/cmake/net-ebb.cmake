set(net_ebb_base ${CMAKE_CURRENT_LIST_DIR}/../../ebb)

file(GLOB net_ebb_source ${net_ebb_base}/src/*.c)

find_package(RAGEL 6.6 REQUIRED)
RAGEL_TARGET(ebb_request_parser ${net_ebb_base}/src/net_ebb_request_parser.rl ${CMAKE_SOURCE_DIR}/net_ebb_request_parser.c)
set(net_ebb_source ${net_ebb_source} ${RAGEL_ebb_request_parser_OUTPUTS})

add_library(net_ebb STATIC ${net_ebb_source})

set_property(TARGET net_ebb PROPERTY INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/pal/include
  ${CMAKE_CURRENT_LIST_DIR}/../../../cpe/utils/include
  ${CMAKE_CURRENT_LIST_DIR}/../../core/include
  ${net_ebb_base}/include
  ${net_ebb_base}/src
  )
