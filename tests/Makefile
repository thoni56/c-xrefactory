# Run all tests
ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

void:
	@echo "'make quick' for only fast tests"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"

all: clean quick slow

quick:
	@echo "Quick tests..."
	@$(MAKE) -C simple_xrefs
	@$(MAKE) -C pragma_once
	@$(MAKE) -C process_yacc
	@$(MAKE) -C yacc_coverage
	@$(MAKE) -C cgram_coverage
	@$(MAKE) -C basic_html_generation
	@$(MAKE) -C standard_includes
	@$(MAKE) -C initializer_parsing
	@$(MAKE) -C generate_str_fill
	@$(MAKE) -C java_example
	@$(MAKE) -C replay_server_example
	@$(MAKE) -C olcx_push
	@$(MAKE) -C complete_coverage

slow:
	@echo "Slow tests..."
	@$(MAKE) -C c-xref
	@$(MAKE) -C emacs_c_exercise
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd

clean:
	-@rm /tmp/c-xref*.log /tmp/c-xref-temp-* /tmp/c-xref*.tmp
