# Run all tests
#
# NOTE: sandboxed_emacs is not a test...

ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

void:
	@echo "'make quick' for only fast tests"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"

all: quick slow

#-------------------------------------------------------------------
# Here follows some GNU magic to automatically discover tests
# It will assume that if there is a directory starting with "test_"
# it is a directory where you can run tests using "make"

# Discover all test directories and a target rule to run all we found
TESTS = $(wildcard test_*)
autodiscovered_tests: $(TESTS)

# Define a macro to generate a RULE for $1
define RULE
.PHONY: $(1)
$(1):
	@$(MAKE) -C $(1)
endef

# Loop over all test_ directories to create a rule for each
$(foreach _t,$(TESTS),$(eval $(call RULE, $(_t))))

#--------------------------------------------------------------------

quick: autodiscovered_tests
	@$(MAKE) -C pragma_once
	@$(MAKE) -C cgram_coverage
	@$(MAKE) -C basic_html_generation
	@$(MAKE) -C standard_includes
	@$(MAKE) -C generate_str_fill
	@$(MAKE) -C edit_server_test_example
	@$(MAKE) -C c_extract_with_argument
	@$(MAKE) -C turn_dynamic_method_to_static
	@$(MAKE) -C extract_with_return
	@$(MAKE) -C java_simple_extract

slow: autodiscovered_tests
	@$(MAKE) -C emacs_c_exercise
	@$(MAKE) -C c-xref
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd
