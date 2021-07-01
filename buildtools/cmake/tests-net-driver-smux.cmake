if (DEFINED net_driver_smux_base)
  file(GLOB test_net_driver_smux_source ${net_driver_smux_base}/tests/*.c)

  add_executable(tests_net_driver_smux ${test_net_driver_smux_source})

  set_property(TARGET tests_net_driver_smux PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${net_driver_smux_base}/include
    ${net_driver_smux_base}/src
    )

  set(tests_net_driver_smux_libraries
    test_utils_cpe_utils test_utils_net_core
    net_driver_smux net_core cpe_utils_sock cpe_utils cpe_pal
    pcre2 cmocka)

  if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64)
    set(tests_net_driver_smux_libraries ${tests_net_driver_smux_libraries} m)
  endif()

  set_property(TARGET tests_net_driver_smux PROPERTY LINK_LIBRARIES ${tests_net_driver_smux_libraries})

  add_test(NAME net-driver-smux COMMAND tests_net_driver_smux)
endif()
