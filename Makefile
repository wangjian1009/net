# {{{ 全局设置
CMAKE?=cmake
CMAKE_MAKE?=make

SANITIZE=address

ifeq ($(IN_DOCKER),1)
CMAKE:=$(CMAKE_ROOT)/bin/cmake
endif


ifeq ($(OS),Windows_NT)
MINGW:=1
CMAKE:=PATH=/mingw64/bin cmake
CMAKE_MAKE:=mingw32-make
CMAKE_GENERATE:=-G "MinGW Makefiles"
endif


# Tools设置
TOOL_BASE:=$(PWD)
TOOL_BUILD_PATH=$(TOOL_BASE)/build/build
TOOL_BIN_PATH=$(TOOL_BASE)/build/run

$(TOOL_BUILD_PATH)/Makefile:
	mkdir -p $(TOOL_BUILD_PATH) \
    && cd $(TOOL_BUILD_PATH) \
    && $(CMAKE) $(TOOL_BASE) \
       -DCMAKE_BUILD_TYPE=$(if $(RELEASE),Release,Debug) \
       -DTOOL_OUTPUT_PATH=$(TOOL_BIN_PATH) $(if $(SANITIZE),-DSANITIZE=$(SANITIZE)) $(if $(GPROF),-DGPROF=$(GPROF)) \
       $(CMAKE_GENERATE)

# Tests设置
CTEST_ROOT:=.
CTEST_OUTPUT:=$(CTEST_ROOT)/build/ctest

$(CTEST_OUTPUT)/Makefile:
	@mkdir -p $(CTEST_OUTPUT) \
	&& cd $(CTEST_OUTPUT) \
	&& cmake -Wno-dev -DBUILD_TESTS=1 -DCMAKE_BUILD_TYPE=Debug \
           $(if $(COV),-DGCOV=$(COV)) \
           $(if $(SANITIZE),-DSANITIZE=address) \
       ../..

# }}}
# {{{ tests

.PHONY: tests
tests: $(CTEST_OUTPUT)/Makefile
	cd $(CTEST_OUTPUT) && make && $(if $V,CTEST_OUTPUT_ON_FAILURE=1) ctest net-http2

# }}}
# {{{ tool-ndt
tool_ndt_output:=$(TOOL_BIN_PATH)/tool_ndt

$(tool_ndt_output): tool_ndt

.PHONY: tool_ndt
tool_ndt: $(TOOL_BUILD_PATH)/Makefile
	$(CMAKE_MAKE) -C $(TOOL_BUILD_PATH) tool_ndt

TOOL_NDT_ARGS+=

run-ndt: $(tool_ndt_output)
	$(tool_ndt_output) $(TOOL_NDT_ARGS)

# }}} 
# {{{ tool-url
TOOL_URL_OUTPUT_PATH:=$(TOOL_BIN_PATH)
tool_url_output:=$(TOOL_URL_OUTPUT_PATH)/tool_url

.PHONY: tool_url
tool_url: $(tool_url_output)

$(tool_url_output): $(TOOL_BUILD_PATH)/Makefile
	$(CMAKE_MAKE) -C $(TOOL_BUILD_PATH) tool_url

TOOL_URL_ARGS+=--request POST
TOOL_URL_ARGS+=--url 'https://52.74.32.119/signal/v2/devices'
TOOL_URL_ARGS+=--header 'Content-Type: application/json'
TOOL_URL_ARGS+=--data '{    "fromUser": "688064",    "fromDeviceId": "ef4169592a8bc05b",    "fromAddress": "0xb6034c685219f457590ebe3798eff801dd88c395",    "fromSeq": 1619091169480,    "_opIdx": 1,    "selection": "AutoMode",    "token": "eyJ1c2VySWQiOjY4ODA2NCwidmlwRXhwaXJlVGltZSI6MCwidXNlclR5cGUiOiJub3JtYWwiLCJfY2hlY2tzdW0iOiJrUUlpcGJwSmhPeWlDam9FY1NMN0ZZOG9pY3czbU50YlRGUFB2emtHVEI2NXBpZVJ6MDhURUZ2UC93TFhBa3ZXUy9POGpKTGJ0NGhoK2UyakR5SGJCK2MxakV2N3hVYWcyTUpGM2xZZWJraGI3OFZBL3MwYmNpN3FPZHN3amlwV0R4OXlLVHdVRXUrSCtaa3lZZXY0OHd4dEhaMk5FSzAzbkNzcXZmZXNQWWM9In0=",    "networkType": "unknown",    "networkMedia": "cellular",    "platform": "Android",    "channel": "googleplay",    "appPkg": "cc.coolline.client",    "appVersion": "1.0.107",    "appName": "CoolVPN",    "imsi": "62120",    "protocols": {        "ss+": {            "crypto-method": [                "aes-256-cfb",                "chacha20",                "chacha20-ietf",                "chacha20-ietf-poly1305",                "xchacha20-ietf-poly1305"            ]        },        "ss": {            "crypto-method": [                "aes-256-cfb",                "chacha20",                "chacha20-ietf",                "chacha20-ietf-poly1305",                "xchacha20-ietf-poly1305"            ]        }    },    "transProtocols": [        "ws",        "wss",        "ssl",        "h2",        "h2s",        "xkcp"    ],    "ignoreDevices": [],    "suggestExclude": []}'

run-url: $(tool_url_output)
	$(tool_url_output) $(TOOL_URL_ARGS)

# }}}
