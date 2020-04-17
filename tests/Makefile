# Run all tests
#
# NOTE: sandboxed_emacs is not a test...

ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

# Create GCOV settings so that tests can be run in parallel by
# directing all coverage to a separate directory under src/.objects
# for each autodiscovered test. If GCOV="" then coverage is written
# to the original object directory (required by Travis build. e.g)
GCOV = GCOV_PREFIX=.coverage GCOV_PREFIX_STRIP=`pwd | awk -F"/" '{print NF}'`

void:
	@echo "'make quick' for only fast tests"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"

all: quick slow

list:
	echo $(TESTS)

clean:
	rm -rf *.coverage */.coverage

#-------------------------------------------------------------------
# Here follows some GNU magic to automatically discover tests
# It will assume that if there is a directory starting with "test_"
# it is a directory where you can run tests using "make"

# Discover all test directories and a target rule to run all we found
TESTS = $(patsubst %/.,%,$(wildcard test_*/.))
autodiscovered_tests: $(TESTS)

# Define a macro to generate a RULE for $1
define RULE
.PHONY: $(1)
$(1):
	@cd $(1) ; lcov -q -z -d .
	@$(MAKE) $(GCOV) -C $(1)
	@if [ ! -z "$(GCOV)" ] ; then \
		if [ -d $(1)/.coverage ] ; then \
			cp ../src/.objects/*.gcno $(1)/.coverage ; \
			lcov -q -t $(1) -d $(1)/.coverage -c -o $(1).coverage ; \
		fi \
	fi
endef

# Loop over all test_ directories to create a rule for each
$(foreach _t,$(TESTS),$(eval $(call RULE,$(_t))))

#--------------------------------------------------------------------

quick: autodiscovered_tests

slow: autodiscovered_tests
	@$(MAKE) -C emacs_c_exercise
	@$(MAKE) -C c-xref
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd
