// swift-tools-version:5.0

import PackageDescription

let package = Package(
    name: "net",
    products: [
        .library(name: "ev", type: .static, targets: ["ev"]),
        .library(name: "c-ares", type: .static, targets: ["c-ares"]),
        .library(name: "mbedtls", type: .static, targets: ["mbedtls"]),
        .library(name: "curl", type: .static, targets: ["curl"]),
        .library(name: "net_core", type: .static, targets: ["net_core"]),
        .library(name: "net_log", type: .static, targets: ["net_log"]),
        .library(name: "net_trans", type: .static, targets: ["net_trans"]),
        .library(name: "net_driver_ev", type: .static, targets: ["net_driver_ev"]),
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
        .target(name: "net_log"
               , path: "log"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
                     .headerSearchPath("../cpe/utils_sock/include"),
                     .headerSearchPath("../cpe/fsm/include"),
                     .headerSearchPath("../cpe/vfs/include"),
                     .headerSearchPath("../depends/mbedtls/include/mbedtls"),
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
        .target(name: "net_driver_ev"
               , path: "driver_ev"
               , sources: ["src"]
               , cSettings: [
                     .headerSearchPath("../cpe/pal/include"),
                     .headerSearchPath("../cpe/utils/include"),
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
        .target(name: "mbedtls"
               , path: "depends/mbedtls"
               , sources: ["library"]
        ),
        .target(name: "ev"
               , path: "depends/libev"
               , sources: ["src/ev.c"]
               , cSettings: [
                     .headerSearchPath("src/ios", .when(platforms: [ .iOS ])),
                     .headerSearchPath("src/mac", .when(platforms: [ .macOS ])),
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
                     .headerSearchPath("../../depends/mbedtls/include"),
                     .headerSearchPath("../../depends/mbedtls/include/mbedtls"),
                     .headerSearchPath("lib"),
                 ]
        ),
    ]
)
