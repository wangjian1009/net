SANITIZE=1
CTEST_ROOT:=.
CTEST_OUTPUT:=$(CTEST_ROOT)/build/ctest

.PHONY: tests
tests: $(CTEST_OUTPUT)/Makefile
	cd $(CTEST_OUTPUT) && make && $(if $V,CTEST_OUTPUT_ON_FAILURE=1) ctest net-http2

$(CTEST_OUTPUT)/Makefile:
	@mkdir -p $(CTEST_OUTPUT) \
	&& cd $(CTEST_OUTPUT) \
	&& cmake -Wno-dev -DBUILD_TESTS=1 -DCMAKE_BUILD_TYPE=Debug \
           $(if $(COV),-DGCOV=$(COV)) \
           $(if $(SANITIZE),-DSANITIZE=address) \
       ../..

