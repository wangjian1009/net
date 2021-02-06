get_filename_component(openssl_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/openssl/${OS_NAME} ABSOLUTE)

#MinGW 准备工作
# ln -s ../usr/bin/git.exe git.exe
# ln -s x86_64-w64-mingw32-gcc-ar.exe x86_64-w64-mingw32-ar.exe
# ln -s x86_64-w64-mingw32-gcc-ranlib.exe x86_64-w64-mingw32-ranlib.exe

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
  include(${CMAKE_CURRENT_LIST_DIR}/scripts/BuildOpenSSL.cmake)
  
  find_package(Git REQUIRED)
  find_package(PythonInterp 3 REQUIRED)

  set(OPENSSL_PREFIX ${openssl_base})
  set(OPENSSL_BUILD_VERSION 1.1.1d)
  set(OPENSSL_BUILD_HASH 1e3a91bc1f9dfce01af26026f856e064eab4c8ee0a8f457b5ae30b40b8b711f2)

  set(CONFIGURE_OPENSSL_MODULES
    no-stdio no-ui-console
    no-cast no-md2 no-md4 no-mdc2 no-rc4 no-rc5 no-engine
    no-idea no-mdc2 no-rc5 no-camellia no-ssl3 no-heartbeats
    no-gost no-deprecated no-capieng no-comp no-dtls no-psk
    no-srp no-rc2 no-shared no-asm no-threads no-dso no-dsa
    no-tests)

  find_program(MAKE_PROGRAM make)

  if (ANDROID)
    set(CONFIGURE_OPENSSL_MODULES ${CONFIGURE_OPENSSL_MODULES} no-hw)

    set(PATH ${ANDROID_TOOLCHAIN_ROOT}/bin/:${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_NAME}/bin/)

    if (ANDROID_ABI MATCHES "arm64-v8a")
      set(COMMAND_CONFIGURE ./Configure android-arm64 --libdir=lib --prefix=${openssl_basef})
    elseif (ANDROID_ABI MATCHES "armeabi-v7a")
      set(COMMAND_CONFIGURE ./Configure android-arm --libdir=lib --prefix=${openssl_base} -march=armv7-a)
    elseif (ANDROID_ABI MATCHES "x86")
      set(COMMAND_CONFIGURE ./Configure android-x86 --libdir=lib --prefix=${openssl_base})
    elseif (ANDROID_ABI MATCHES "x86_64")
      set(COMMAND_CONFIGURE ./Configure android-x86_64 --libdir=lib --prefix=${openssl_base})
    endif()

    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py LINUX_CROSS_ANDROID)

    # write environment to file, is picked up by python script
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
      if (NOT _variableName MATCHES "lines")
        set(OUT_FILE "${OUT_FILE}${_variableName}=\"${${_variableName}}\"\n")
      endif()
    endforeach()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/buildenv.txt ${OUT_FILE})
    
    BuildOpenSSL(PROJECT openssl OUTPUT ${openssl_base})

  elseif (IOS)
    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py IOS)

    set(COMMAND_CONFIGURE ./Configure iossimulator-xcrun --libdir=lib/libsimulator --prefix=${openssl_base})
    BuildOpenSSL(PROJECT openssl-iossimulator OUTPUT ${openssl_base})
    list(APPEND OPENSSL_LIBSSL_INPUTS ${openssl_base}/lib/libsimulator/libssl.a)
    list(APPEND OPENSSL_LIBCRYPTO_INPUTS ${openssl_base}/lib/libsimulator/libcrypto.a)
    
    set(COMMAND_CONFIGURE ./Configure ios64-xcrun -fembed-bitcode --libdir=lib/lib64 --prefix=${openssl_base})
    BuildOpenSSL(PROJECT openssl-ios64 OUTPUT ${openssl_base})
    list(APPEND OPENSSL_LIBSSL_INPUTS ${openssl_base}/lib/lib64/libssl.a)
    list(APPEND OPENSSL_LIBCRYPTO_INPUTS ${openssl_base}/lib/lib64/libcrypto.a)
    
    set(COMMAND_CONFIGURE ./Configure ios-xcrun -fembed-bitcode --libdir=lib/lib32 --prefix=${openssl_base})
    BuildOpenSSL(PROJECT openssl-ios32 OUTPUT ${openssl_base})
    list(APPEND OPENSSL_LIBSSL_INPUTS ${openssl_base}/lib/lib32/libssl.a)
    list(APPEND OPENSSL_LIBCRYPTO_INPUTS ${openssl_base}/lib/lib32/libcrypto.a)
    
    add_custom_target(openssl
      COMMAND lipo -output ${OPENSSL_LIBSSL_PATH} -create ${OPENSSL_LIBSSL_INPUTS}
      COMMAND lipo -output ${OPENSSL_LIBCRYPTO_PATH} -create ${OPENSSL_LIBCRYPTO_INPUTS}
      )
    add_dependencies(openssl openssl-ios64)
    add_dependencies(openssl openssl-ios32)
    add_dependencies(openssl openssl-iossimulator)
  elseif (MINGW)
    set(COMMAND_CONFIGURE ./Configure mingw64 --cross-compile-prefix=x86_64-w64-mingw32-
      --libdir=lib --prefix=${openssl_base})
    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py
      WIN32 c:/msys64/usr/bin/bash.exe c:/msys64/mingw64)
    BuildOpenSSL(PROJECT openssl OUTPUT ${openssl_base})
  else()
    set(COMMAND_CONFIGURE ./config --libdir=lib --prefix=${openssl_base})
    set(BUILD_ENV_TOOL ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/building_env.py UNIX)
    BuildOpenSSL(PROJECT openssl OUTPUT ${openssl_base})
  endif()

  add_library(ssl_lib STATIC IMPORTED GLOBAL)
  add_library(crypto_lib STATIC IMPORTED GLOBAL)

  add_dependencies(ssl ssl_lib)
  add_dependencies(crypto crypto_lib)
  
  target_link_libraries(ssl INTERFACE ssl_lib)
  target_link_libraries(crypto INTERFACE crypto_lib)
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
elseif (OS_NAME STREQUAL mingw)
  target_link_libraries(crypto INTERFACE ws2_32)
endif()
