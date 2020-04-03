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
	# DISABLED - we are going to remove it : @$(MAKE) -C generate_str_fill
	@$(MAKE) -C jar_reading
	@$(MAKE) -C java_example
	@$(MAKE) -C partial_jexercise
	@$(MAKE) -C edit_server_test_example
	@$(MAKE) -C push
	@$(MAKE) -C complete
	@$(MAKE) -C rename
	@$(MAKE) -C simple_extract
	@$(MAKE) -C add_parameter
	@$(MAKE) -C move_class
	@$(MAKE) -C turn_dynamic_method_to_static
	@$(MAKE) -C extract_with_return

slow:
	@$(MAKE) -C emacs_c_exercise
	@$(MAKE) -C c-xref
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd
