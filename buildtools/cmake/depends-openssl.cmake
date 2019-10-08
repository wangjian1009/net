set(BUILD_OPENSSL ON)
set(OPENSSL_BUILD_VERSION "1.1.1d")
set(OPENSSL_INSTALL_MAN OFF)
set(CROSS_ANDROID ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../depends/openssl openssl.out)
