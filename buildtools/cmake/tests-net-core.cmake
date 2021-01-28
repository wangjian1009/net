file(GLOB test_net_core_source ${net_core_base}/tests/*.c)

add_executable(tests_net_core ${test_net_core_source})

set_property(TARGET tests_net_core PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${cpe_pal_base}/include
  ${cpe_utils_base}/include
  ${cpe_utils_base}/test_utils
  ${cpe_utils_yaml_base}/include
  ${net_core_base}/include
  ${net_core_base}/test_utils
  )

set(tests_net_core_libraries
  test_utils_cpe_utils test_utils_net_core
  net_core cpe_utils_yaml cpe_utils_sock cpe_utils cpe_pal yaml pcre2 check cmocka)

if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
  set(tests_net_core_libraries ${tests_net_core_libraries} m)
endif()

set_property(TARGET tests_net_core PROPERTY LINK_LIBRARIES ${tests_net_core_libraries})

add_test(NAME net-core COMMAND tests_net_core)
