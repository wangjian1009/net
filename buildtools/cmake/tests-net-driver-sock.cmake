if(DEFINED net_driver_sock_base)
  file(GLOB test_net_driver_sock_source ${net_driver_sock_base}/tests/*.c)

  add_executable(tests_net_driver_sock ${test_net_driver_sock_source})

  set_property(TARGET tests_net_driver_sock PROPERTY INCLUDE_DIRECTORIES
    ${cmocka_base}/include
    ${cpe_pal_base}/include
    ${cpe_utils_base}/include
    ${cpe_utils_base}/test_utils
    ${net_core_base}/include
    ${net_core_base}/test_utils
    ${ev_base}/include
    ${net_driver_ev_base}/include
    ${net_driver_sock_base}/include
    ${net_driver_sock_base}/src
    ${net_driver_sock_base}/test_utils
    )

  set(tests_net_driver_sock_libraries
    test_utils_net_core
    net_driver_ev cmocka)

  set_property(TARGET tests_net_driver_sock PROPERTY LINK_LIBRARIES ${tests_net_driver_sock_libraries})

  add_test(NAME net-driver-sock COMMAND tests_net_driver_sock)
endif()
