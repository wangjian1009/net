#test_utils
include(${CMAKE_CURRENT_LIST_DIR}/test-utils-net-core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/test-utils-net-driver-ws.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/test-utils-net-protocol-http.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/test-utils-net-protocol-http-svr.cmake)

#tests
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-dns.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-ssl.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-ws.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-http2.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-smux.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-xkcp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-protocol-http.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-prometheus.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-prometheus-process.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-prometheus-http.cmake)
