set(libevent_base ${CMAKE_CURRENT_LIST_DIR}/../../depends/libevent)
set(libevent_tmp ${CMAKE_CURRRENT_BINARY_DIR}/include/libevent)

set(libevent_source
  ${libevent_base}/buffer.c
  ${libevent_base}/bufferevent.c
  ${libevent_base}/bufferevent_filter.c
  ${libevent_base}/bufferevent_pair.c
  ${libevent_base}/bufferevent_ratelim.c
  ${libevent_base}/bufferevent_sock.c
  ${libevent_base}/event.c
  ${libevent_base}/evmap.c
  ${libevent_base}/evthread.c
  ${libevent_base}/evutil.c
  ${libevent_base}/evutil_rand.c
  ${libevent_base}/evutil_time.c
  ${libevent_base}/watch.c
  ${libevent_base}/listener.c
  ${libevent_base}/log.c
  ${libevent_base}/signal.c
  ${libevent_base}/strlcpy.c)

if (OS_NAME STREQUAL linux32 OR OS_NAME STREQUAL linux64 OR OS_NAME STREQUAL android)
  list(APPEND libevent_source
    ${libevent_base}/epoll.c)
elseif (OS_NAME STREQUAL mac OR OS_NAME STREQUAL ios)
  list(APPEND libevent_source
    ${libevent_base}/kqueue.c)
endif()

add_library(libevent STATIC ${libevent_source})
set_property(TARGET libevent PROPERTY COMPILE_OPTIONS ${libevent_compile_options})
set_property(TARGET libevent PROPERTY COMPILE_DEFINITIONS ${libevent_compile_definitions})

configure_file(
    ${libevent_base}/event-config.h.cmake
    ${libevent_tmp}/event2/event-config.h
        NEWLINE_STYLE UNIX)

configure_file(
    ${libevent_base}/evconfig-private.h.cmake
    ${libevent_tmp}/private/evconfig-private.h)

set_property(TARGET libevent PROPERTY INCLUDE_DIRECTORIES
  ${libevent_base}/include
  ${libevent_tmp}
  ${libevent_tmp}/private
  ${openssl_base}/include
  )
