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

run-tool: $(tool_url_output)
	$(tool_url_output) --url=https://www.baidu.com

$(OUTPUT_PATH)/Makefile:
	mkdir -p $(OUTPUT_PATH) \
    && cd $(OUTPUT_PATH) \
    && $(CMAKE) $(PROJECT_BASE) \
       -DCMAKE_BUILD_TYPE=$(if $(RELEASE),Release,Debug) \
       -DTOOL_URL_OUTPUT_PATH=$(TOOL_URL_OUTPUT_PATH) $(if $(SANITIZE),-DSANITIZE=$(SANITIZE)) $(if $(GPROF),-DGPROF=$(GPROF)) \
       $(CMAKE_GENERATE)
