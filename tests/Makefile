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

quick:
	@$(MAKE) -C simple_xrefs
	@$(MAKE) -C pragma_once
	@$(MAKE) -C process_yacc
	@$(MAKE) -C yacc_coverage
	@$(MAKE) -C cgram_coverage
	@$(MAKE) -C basic_html_generation
	@$(MAKE) -C standard_includes
	@$(MAKE) -C initializer_parsing
	@$(MAKE) -C generate_str_fill
	@$(MAKE) -C jar_reading
	@$(MAKE) -C java_example
	@$(MAKE) -C edit_server_test_example
	@$(MAKE) -C olcx_push
	@$(MAKE) -C olcx_complete
	@$(MAKE) -C olcx_refactory_rename
	@$(MAKE) -C olcx_refactory_simple_extract
	@$(MAKE) -C olcx_refactory_add_parameter
	@$(MAKE) -C emacs_c_exercise

slow:
	@$(MAKE) -C c-xref
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd
