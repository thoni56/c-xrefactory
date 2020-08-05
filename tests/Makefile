# Run all tests
#
# NOTE: sandboxed_emacs is not a test...

.SILENT:

ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

# Create GCOV settings so that tests can be run in parallel by
# directing all coverage to a separate directory under src/.objects
# for each autodiscovered test. If GCOV_OPTIONS="" then coverage is
# written to the original object directory (required by Travis
# build. e.g) which means we cannot run tests in parallel
GCOV_OPTIONS = GCOV_PREFIX=$(CURDIR)/$(1)/.coverage GCOV_PREFIX_STRIP=`pwd | awk -F"/" '{print NF}'`

void:
	@echo "'make quick' for only fast tests"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"

all: quick slow

list:
	echo $(TESTS)

clean:
	rm -rf *.info */.coverage

#-------------------------------------------------------------------
# Here follows some GNU magic to automatically discover tests
# It will assume that if there is a directory starting with "test_"
# it is a directory where you can run tests using "make"

# Discover all test directories and a target rule to run all we found
TESTS = $(patsubst %/.,%,$(wildcard test_*/.))
autodiscovered_tests: $(TESTS)

# Define a macro to generate a RULE for a test(directory)
# In case we run the tests with -j all tests need to store
# coverage data in their own directories as per above
define RULE
.PHONY: $(1)
$(1):
	mkdir -p $(1)/.coverage
	if command -v lcov > /dev/null 2>&1 ; then \
		cd $(1) ; \
		lcov -q -z -d . ; \
	fi
	$(MAKE) $(GCOV_OPTIONS) -C $(1)
	if command -v lcov > /dev/null 2>&1 ; then \
		if [ ! -z "$(GCOV_OPTIONS)" ] ; then \
			if [ "$(ls -A $(1)/.coverage)" ] ; then \
				cd ../src ; \
				cp .objects/*.gcno ../tests/$(1)/.coverage ; \
				lcov -q -t $(1) -d ../tests/$(1)/.coverage -c -o ../tests/$(1).info ; \
			fi \
		fi \
	fi
endef

# Loop over all test_ directories to create a rule for each
$(foreach _t,$(TESTS),$(eval $(call RULE,$(_t))))

#--------------------------------------------------------------------

quick: autodiscovered_tests

slow:
	$(MAKE) -C c-xref
	$(MAKE) -C ffmpeg
	$(MAKE) -C systemd
