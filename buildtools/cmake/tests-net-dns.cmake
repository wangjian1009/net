if (DEFINED net_dns_base)
  file(GLOB tests_net_dns_source ${net_dns_base}/tests/*.c)

  add_executable(tests_net_dns ${tests_net_dns_source})

  set_property(TARGET tests_net_dns PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${yaml_base}/include
    ${cpe_utils_yaml_base}/include
    ${net_core_base}/include
    ${net_dns_base}/include
    )

  set(tests_net_dns_libraries net_dns test_utils_cpe_utils cpe_utils_yaml)

  if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
    set(tests_net_dns_libraries ${tests_net_dns_libraries} m)
  endif()

  set_property(TARGET tests_net_dns PROPERTY LINK_LIBRARIES ${tests_net_dns_libraries})

  add_test(NAME net-dns COMMAND tests_net_dns)
endif()
