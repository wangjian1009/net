#test_utils
include(${CMAKE_CURRENT_LIST_DIR}/test-utils-net-core.cmake)

#tests
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-dns.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-ssl.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-driver-ws.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/tests-net-protocol-http.cmake)
