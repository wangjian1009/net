set(openssl_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/openssl/${OS_NAME})

if (ANDROID)
  set(openssl_base "${openssl_base}/${ANDROID_ABI}")
endif()

set(OPENSSL_LIBSSL_PATH ${openssl_base}/lib/libssl.a)
set(OPENSSL_LIBCRYPTO_PATH ${openssl_base}/lib/libcrypto.a)

add_library(ssl INTERFACE)
add_library(crypto INTERFACE)
target_include_directories(ssl BEFORE INTERFACE ${openssl_base}/include)
target_include_directories(crypto BEFORE INTERFACE ${openssl_base}/include)

if (BUILD_OPENSSL)
  include(ExternalProject)

  find_package(Git REQUIRED)
  find_package(PythonInterp 3 REQUIRED)

  set(OPENSSL_PREFIX ${openssl_base})
  set(OPENSSL_BUILD_VERSION 1.1.1d)
  set(OPENSSL_BUILD_HASH 1e3a91bc1f9dfce01af26026f856e064eab4c8ee0a8f457b5ae30b40b8b711f2)

  add_library(ssl_lib STATIC IMPORTED GLOBAL)
  add_library(crypto_lib STATIC IMPORTED GLOBAL)

  target_link_libraries(ssl INTERFACE ssl_lib)
  target_link_libraries(crypto INTERFACE crypto_lib)

  set(PERL_PATH_FIX_INSTALL true)

  set(CONFIGURE_OPENSSL_MODULES
    no-stdio no-ui-console
    no-cast no-md2 no-md4 no-mdc2 no-rc4 no-rc5 no-engine
    no-idea no-mdc2 no-rc5 no-camellia no-ssl3 no-heartbeats
    no-gost no-deprecated no-capieng no-comp no-dtls no-psk
    no-srp no-rc2 no-shared no-asm no-threads no-dso no-dsa
    no-tests)

  set(CONFIGURE_OPENSSL_PARAMS --libdir=lib --prefix=${openssl_base})
  
  if (ANDROID)
    set(CONFIGURE_OPENSSL_MODULES ${CONFIGURE_OPENSSL_MODULES} no-hw)

    set(PATH ${ANDROID_TOOLCHAIN_ROOT}/bin/:${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_NAME}/bin/)

    if (ANDROID_ABI MATCHES "arm64-v8a")
      set(COMMAND_CONFIGURE ./Configure android-arm64 ${CONFIGURE_OPENSSL_PARAMS} ${CONFIGURE_OPENSSL_MODULES})
    elseif (ANDROID_ABI MATCHES "armeabi-v7a")
      set(COMMAND_CONFIGURE ./Configure android-arm ${CONFIGURE_OPENSSL_PARAMS} -march=armv7-a ${CONFIGURE_OPENSSL_MODULES})
    endif()

    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py LINUX_CROSS_ANDROID)
  elseif (IOS)
    set(COMMAND_CONFIGURE ./Configure ios64-xcrun ${CONFIGURE_OPENSSL_PARAMS} ${CONFIGURE_OPENSSL_MODULES})
    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py IOS)
  else()
    set(COMMAND_CONFIGURE ./config ${CONFIGURE_OPENSSL_PARAMS} ${CONFIGURE_OPENSSL_MODULES})
    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py UNIX)
  endif()

  find_program(MAKE_PROGRAM make)

  message(STATUS "COMMAND_CONFIGURE=${COMMAND_CONFIGURE}")
  message(STATUS "BUILD_ENV_TOOL=${BUILD_ENV_TOOL}")
  message(STATUS "MAKE_PROGRAM=${MAKE_PROGRAM}")
  
  # add openssl target
  ExternalProject_Add(openssl
    PREFIX openssl
    DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}/../../build
    INSTALL_DIR ${openssl_base}
    URL https://mirror.viaduck.org/openssl/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
    URL_HASH SHA256=${OPENSSL_BUILD_HASH}
    UPDATE_COMMAND ""

    CONFIGURE_COMMAND ${BUILD_ENV_TOOL} <SOURCE_DIR> ${COMMAND_CONFIGURE}

    BUILD_COMMAND ${BUILD_ENV_TOOL} <SOURCE_DIR> ${MAKE_PROGRAM} -j ${NUM_JOBS}
    BUILD_BYPRODUCTS ${OPENSSL_LIBSSL_PATH} ${OPENSSL_LIBCRYPTO_PATH}

    # TEST_BEFORE_INSTALL 1
    # TEST_COMMAND ${COMMAND_TEST}

    INSTALL_COMMAND ${BUILD_ENV_TOOL} <SOURCE_DIR> ${PERL_PATH_FIX_INSTALL}
    COMMAND ${BUILD_ENV_TOOL} <SOURCE_DIR> ${MAKE_PROGRAM} install_sw #DESTDIR=${OPENSSL_PREFIX} 
    COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} ${CMAKE_BINARY_DIR}                    # force CMake-reload

    LOG_INSTALL 1
    )

  # set git config values to openssl requirements (no impact on linux though)
  ExternalProject_Add_Step(openssl setGitConfig
    COMMAND ${GIT_EXECUTABLE} config --global core.autocrlf false
    COMMAND ${GIT_EXECUTABLE} config --global core.eol lf
    DEPENDEES
    DEPENDERS download
    ALWAYS ON
    )

  # set, don't abort if it fails (due to variables being empty). To realize this we must only call git if the configs
  # are set globally, otherwise do a no-op command ("echo 1", since "true" is not available everywhere)
  if (GIT_CORE_AUTOCRLF)
    set (GIT_CORE_AUTOCRLF_CMD ${GIT_EXECUTABLE} config --global core.autocrlf ${GIT_CORE_AUTOCRLF})
  else()
    set (GIT_CORE_AUTOCRLF_CMD echo)
  endif()
  if (GIT_CORE_EOL)
    set (GIT_CORE_EOL_CMD ${GIT_EXECUTABLE} config --global core.eol ${GIT_CORE_EOL})
  else()
    set (GIT_CORE_EOL_CMD echo)
  endif()
  ##

  # set git config values to previous values
  ExternalProject_Add_Step(openssl restoreGitConfig
    # unset first (is required, since old value could be omitted, which wouldn't take any effect in "set"
    COMMAND ${GIT_EXECUTABLE} config --global --unset core.autocrlf
    COMMAND ${GIT_EXECUTABLE} config --global --unset core.eol

    COMMAND ${GIT_CORE_AUTOCRLF_CMD}
    COMMAND ${GIT_CORE_EOL_CMD}

    DEPENDEES download
    DEPENDERS configure
    ALWAYS ON
    )

  # write environment to file, is picked up by python script
  get_cmake_property(_variableNames VARIABLES)
  foreach (_variableName ${_variableNames})
    if (NOT _variableName MATCHES "lines")
      set(OUT_FILE "${OUT_FILE}${_variableName}=\"${${_variableName}}\"\n")
    endif()
  endforeach()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/buildenv.txt ${OUT_FILE})

  set_target_properties(ssl_lib PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIBSSL_PATH})
  set_target_properties(crypto_lib PROPERTIES IMPORTED_LOCATION ${OPENSSL_LIBCRYPTO_PATH})

  add_dependencies(ssl_lib openssl)
  add_dependencies(crypto_lib openssl)
else()
  target_link_libraries(ssl INTERFACE ${OPENSSL_LIBSSL_PATH})
  target_link_libraries(crypto INTERFACE ${OPENSSL_LIBCRYPTO_PATH})
endif()

if (OS_NAME MATCHES "linux")
  target_link_libraries(crypto INTERFACE dl)
endif()
