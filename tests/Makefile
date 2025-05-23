# Run all tests
#
# To be able to run tests in parallel we generate coverage files in each test directory's .coverage.
# To aggregate them there are two ways:
# 1. if there is a gcov-tool available this can merge `.gcda` files directly (fast), otherwise
# 2. create a separate coverage.info for each test and merge them using lcov (slow)
#
# The magic of this is in `run_test` which runs the actual test, reports result on screen and collects coverage

OS := $(shell uname)
ifeq ($(OS),Darwin)
	ifeq ($(CC), gcc-14)
		LCOV=lcov --gcov-tool=gcov-14
	else
		LCOV=/Applications/Xcode.app//Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/llvm-cov
	endif
else
	LCOV=lcov
endif

.PHONY: void
void:
	@echo "'make quick' for only fast tests"
	@echo "'make verbose' for each test separately with results"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"


#-------------------------------------------------------------------
# Here follows some GNU magic to automatically discover tests
# It will assume that if there is a directory starting with "test_"
# it is a directory where you can run tests using "make"

QUICK_TESTS = $(shell find . -maxdepth 1 -name 'test_*' -type d ! -exec test -e {}/.slow \; -print)
SLOW_TESTS = $(shell find . -maxdepth 1 -name 'test_*' -type d -exec test -e {}/.slow \; -print)
ALL_TESTS = $(QUICK_TESTS) $(SLOW_TESTS)


.PHONY: verbose
verbose:
	$(foreach dir,$(TESTS),\
		/bin/echo -n "$(dir): "; \
		./run_test $(dir); \
		/bin/echo; \
    )

.PHONY: quick
quick: $(QUICK_TESTS)

.PHONY: slow
slow: $(SLOW_TESTS)

.PHONY: all
all: $(ALL_TESTS)

.PHONY: list
list:
	echo $(ALL_TESTS) | tr ' ' '\n' | sed 's%./test%test%' | sort

.PHONY: clean
clean:
	$(foreach dir,$(ALL_TESTS),\
	if [ ! -f $(dir)/.suspended ]; then \
		$(MAKE) -C $(dir) clean ; \
	fi; \
    )

# Be silent unless VERBOSE is set to something (VERBOSE=1)
$(VERBOSE).SILENT:

ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif


# To make it possible to run tests in parallel we use GCOV settings so
# that all coverage is directed to a separate directory for each
# autodiscovered test. If not running in parallel then GCOV_OPTIONS
# can be empty to make coverage written to the original object
# directory (required by Travis build as set in .travis.yml)

# Define a macro to generate a RULE for a test(directory)
# In case we run the tests with -j all tests need to store
# coverage data in their own directories as per above
define RULE
.PHONY: $(1)
$(1):
	+GCOV_OPTIONS="GCOV_PREFIX=$(CURDIR)/$(1)/.cov GCOV_PREFIX_STRIP=`pwd | awk -F"/" '{print NF+1}'`" ./run_test $(1)
endef

# Loop over all test_ directories to create a rule for each
$(foreach _t,$(ALL_TESTS),$(eval $(call RULE,$(_t))))
