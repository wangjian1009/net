file(GLOB test_utils_net_core_source ${net_core_base}/test_utils/*.c)

add_library(test_utils_net_core STATIC ${test_utils_net_core_source})

set_property(TARGET test_utils_net_core PROPERTY INCLUDE_DIRECTORIES
  ${cmocka_base}/include
  ${net_core_includes}
  ${cpe_utils_base}/test_utils
  )
