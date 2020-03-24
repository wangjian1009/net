// swift-tools-version:5.0

import PackageDescription

let package = Package(
    name: "net",
    products: [
        .library(name: "libevent", type: .static, targets: ["libevent"]),
        .library(name: "c-ares", type: .static, targets: ["c-ares"]),
        .library(name: "curl", type: .static, targets: ["curl"]),
        .library(name: "net_core", type: .static, targets: ["net_core"]),
        .library(name: "net_log", type: .static, targets: ["net_log"]),
        .library(name: "net_ebb", type: .static, targets: ["net_ebb"]),
        .library(name: "net_ping", type: .static, targets: ["net_ping"]),
        .library(name: "net_trans", type: .static, targets: ["net_trans"]),
        .library(name: "net_driver_libevent", type: .static, targets: ["net_driver_libevent"]),
        .library(name: "net_driver_dq", type: .static, targets: ["net_driver_dq"]),
        .library(name: "net_driver_sock", type: .static, targets: ["net_driver_sock"]),
    ],
    targets: [
        .target(name: "net_core"
               , path: "core"
               , sources: ["src"]
               , cSettings: [
                     .define("PCRE2_STATIC"),
                     .define("PCRE2_CODE_UNIT_WIDTH", to: "8"),
                     .headerSearchPath("../cpe/depends/pcre2/include"),
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                 ]
        ),
        .target(name: "net_trans"
               , path: "trans"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../depends/curl/include"),
                     .headerSearchPath("../core/include"),
                 ]
        ),
        .target(name: "net_ebb"
               , path: "ebb"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../core/include"),
                 ]
        ),
        .target(name: "net_ping"
               , path: "ping"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../core/include"),
                     .headerSearchPath("../trans/include"),
                 ]
        ),
        .target(name: "net_log"
               , path: "log"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../cpe/fsm/include"),
                     .headerSearchPath("../cpe/vfs/include"),
                     .headerSearchPath("../depends/libev/include"),
                     .headerSearchPath("../core/include"),
                     .headerSearchPath("../trans/include"),
                     .headerSearchPath("../driver_ev/include"),
                 ]
        ),
        .target(name: "net_driver_sock"
               , path: "driver_sock"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../core/include"),
                 ]
        ),
        .target(name: "net_driver_libevent"
               , path: "driver_libevent"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../depends/libevent/include"),
                     .headerSearchPath("../core/include"),
                     .headerSearchPath("../driver_sock/include"),
                 ]
        ),
        .target(name: "net_driver_dq"
               , path: "driver_dq"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../core/include"),
                     .headerSearchPath("../driver_sock/include"),
                 ]
        ),
        .target(name: "libevent"
               , path: "."
               , sources: [
                     "depends/libevent/buffer.c",
                     "depends/libevent/bufferevent.c",
                     "depends/libevent/bufferevent_filter.c",
                     "depends/libevent/bufferevent_pair.c",
                     "depends/libevent/bufferevent_ratelim.c",
                     "depends/libevent/bufferevent_sock.c",
                     "depends/libevent/bufferevent_openssl.c",
                     "depends/libevent/event.c",
                     "depends/libevent/evmap.c",
                     "depends/libevent/evthread.c",
                     "depends/libevent/evutil.c",
                     "depends/libevent/evutil_rand.c",
                     "depends/libevent/evutil_time.c",
                     "depends/libevent/watch.c",
                     "depends/libevent/listener.c",
                     "depends/libevent/log.c",
                     "depends/libevent/signal.c",
                     "depends/libevent/strlcpy.c",
                     "depends/libevent/kqueue.c",
                 ]
               , cSettings: [
                     .headerSearchPath("depends/libevent/include"),
                     .headerSearchPath("buildtools/custom/libevent"),
                     .headerSearchPath("buildtools/custom/libevent/ios", .when(platforms: [ .iOS ])),
                     .headerSearchPath("buildtools/custom/libevent/ios/private", .when(platforms: [ .iOS ])),
                     .headerSearchPath("buildtools/custom/libevent/mac", .when(platforms: [ .macOS ])),
                     .headerSearchPath("buildtools/custom/libevent/mac/private", .when(platforms: [ .macOS ])),
                 ]
        ),
        .target(name: "c-ares"
               , path: "depends/c-ares"
               , exclude: ["src/acountry.c", "src/ahost.c"]
               , sources: ["src"]
               , cSettings: [
                     .define("HAVE_CONFIG_H"),
                     .headerSearchPath("include/ios", .when(platforms: [ .iOS ])),
                     .headerSearchPath("include/mac", .when(platforms: [ .macOS ])),
                     .headerSearchPath("src/ios", .when(platforms: [ .iOS ])),
                     .headerSearchPath("src/mac", .when(platforms: [ .macOS ])),
                 ]
        ),
        .target(name: "curl"
               , path: "depends/curl"
               , sources: ["lib"]
               , cSettings: [
                     .define("HAVE_CONFIG_H"),
                     .define("BUILDING_LIBCURL"),
                     .headerSearchPath("../../depends/c-ares/include"),
                     .headerSearchPath("../../depends/c-ares/include/ios", .when(platforms: [ .iOS ])),
                     .headerSearchPath("../../depends/c-ares/include/mac", .when(platforms: [ .macOS ])),
                     .headerSearchPath("../../depends/openssl/include"),
                     .headerSearchPath("../../depends/nghttp2/lib/includes"),
                     .headerSearchPath("../../buildtools/custom/nghttp2"),
                     .headerSearchPath("lib"),
                 ]
        ),
    ]
)
