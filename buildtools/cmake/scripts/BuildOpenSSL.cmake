function( BuildOpenSSL )
  set( options )
  set( oneValueArgs PROJECT OUTPUT)
  set( multiValueArgs )
  cmake_parse_arguments( BuildOpenSSL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( NOT DEFINED BuildOpenSSL_PROJECT )
    message( "BuildOpenSSL PROJECT not defined - skipping" )
    return()
  endif()

  if( NOT DEFINED BuildOpenSSL_OUTPUT )
    message( "BuildOpenSSL OUTPUT not defined - skipping" )
    return()
  endif()

  # add openssl target
  ExternalProject_Add(${BuildOpenSSL_PROJECT}
    PREFIX openssl
    DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}/../../build
    INSTALL_DIR ${BuildOpenSSL_OUTPUT}
    URL https://mirror.viaduck.org/openssl/openssl-${OPENSSL_BUILD_VERSION}.tar.gz
    URL_HASH SHA256=${OPENSSL_BUILD_HASH}

    UPDATE_COMMAND
    ${BUILD_ENV_TOOL} <SOURCE_DIR> sed -i -e 's/return return/return/g' crypto/threads_none.c

    CONFIGURE_COMMAND
    ${BUILD_ENV_TOOL} <SOURCE_DIR> ${COMMAND_CONFIGURE} ${CONFIGURE_OPENSSL_MODULES}

    BUILD_COMMAND
    ${BUILD_ENV_TOOL} <SOURCE_DIR> ${MAKE_PROGRAM} -j ${NUM_JOBS} &&
    ${BUILD_ENV_TOOL} <SOURCE_DIR> ${MAKE_PROGRAM} install_sw #DESTDIR=${OPENSSL_PREFIX}
    
    BUILD_BYPRODUCTS ${BuildOpenSSL_OUTPUT}/lib/libssl.a ${BuildOpenSSL_OUTPUT}/lib/libcrypto.a

    INSTALL_COMMAND
    ${BUILD_ENV_TOOL} <SOURCE_DIR> true
    )

endfunction( BuildOpenSSL )
