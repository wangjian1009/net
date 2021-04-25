.PHONY: tool_url
CMAKE?=cmake
CMAKE_MAKE?=make

ifeq ($(IN_DOCKER),1)
CMAKE:=$(CMAKE_ROOT)/bin/cmake
endif

PROJECT_BASE:=$(PWD)

TOOL_URL_OUTPUT_BASE=$(PROJECT_BASE)/../build
TOOL_URL_OUTPUT_PATH=$(TOOL_URL_OUTPUT_BASE)/run
OUTPUT_PATH:=$(TOOL_URL_OUTPUT_BASE)/build
tool_url_output:=$(TOOL_URL_OUTPUT_PATH)/tool_url

ifeq ($(OS),Windows_NT)
MINGW:=1
CMAKE:=PATH=/mingw64/bin cmake
CMAKE_MAKE:=mingw32-make
CMAKE_GENERATE:=-G "MinGW Makefiles"
endif

tool_url: $(OUTPUT_PATH)/Makefile
	$(CMAKE_MAKE) -C $(OUTPUT_PATH)

$(tool_url_output): tool_url

TOOL_ARGS+=--request POST
TOOL_ARGS+=--url 'https://172.18.6.53/signal/v2/devices'
TOOL_ARGS+=--header 'Content-Type: application/json'
TOOL_ARGS+=--data '{    "fromUser": "688064",    "fromDeviceId": "ef4169592a8bc05b",    "fromAddress": "0xb6034c685219f457590ebe3798eff801dd88c395",    "fromSeq": 1619091169480,    "_opIdx": 1,    "selection": "AutoMode",    "token": "eyJ1c2VySWQiOjY4ODA2NCwidmlwRXhwaXJlVGltZSI6MCwidXNlclR5cGUiOiJub3JtYWwiLCJfY2hlY2tzdW0iOiJrUUlpcGJwSmhPeWlDam9FY1NMN0ZZOG9pY3czbU50YlRGUFB2emtHVEI2NXBpZVJ6MDhURUZ2UC93TFhBa3ZXUy9POGpKTGJ0NGhoK2UyakR5SGJCK2MxakV2N3hVYWcyTUpGM2xZZWJraGI3OFZBL3MwYmNpN3FPZHN3amlwV0R4OXlLVHdVRXUrSCtaa3lZZXY0OHd4dEhaMk5FSzAzbkNzcXZmZXNQWWM9In0=",    "networkType": "unknown",    "networkMedia": "cellular",    "platform": "Android",    "channel": "googleplay",    "appPkg": "cc.coolline.client",    "appVersion": "1.0.107",    "appName": "CoolVPN",    "imsi": "62120",    "protocols": {        "ss+": {            "crypto-method": [                "aes-256-cfb",                "chacha20",                "chacha20-ietf",                "chacha20-ietf-poly1305",                "xchacha20-ietf-poly1305"            ]        },        "ss": {            "crypto-method": [                "aes-256-cfb",                "chacha20",                "chacha20-ietf",                "chacha20-ietf-poly1305",                "xchacha20-ietf-poly1305"            ]        }    },    "transProtocols": [        "ws",        "wss",        "ssl",        "h2",        "h2s",        "xkcp"    ],    "ignoreDevices": [],    "suggestExclude": []}'

run-tool: $(tool_url_output)
	$(tool_url_output) $(TOOL_ARGS)

$(OUTPUT_PATH)/Makefile:
	mkdir -p $(OUTPUT_PATH) \
    && cd $(OUTPUT_PATH) \
    && $(CMAKE) $(PROJECT_BASE) \
       -DCMAKE_BUILD_TYPE=$(if $(RELEASE),Release,Debug) \
       -DTOOL_URL_OUTPUT_PATH=$(TOOL_URL_OUTPUT_PATH) $(if $(SANITIZE),-DSANITIZE=$(SANITIZE)) $(if $(GPROF),-DGPROF=$(GPROF)) \
       $(CMAKE_GENERATE)
