# Run all tests
ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

void:
	@echo "'make quick' for only fast tests"
	@echo "'make slow' for only slow tests"
	@echo "'make all' for both fast and slow tests"

all: quick slow

quick:
	@echo "Quick tests..."
	@$(MAKE) -C ellipsis
	@$(MAKE) -C simple_xrefs
	@$(MAKE) -C pragma_once
	@$(MAKE) -C process_yacc
	@$(MAKE) -C basic_html_generation
	@$(MAKE) -C standard_includes
	@$(MAKE) -C initializer_parsing
	@$(MAKE) -C generate_str_fill
	@$(MAKE) -C java_example
	#@$(MAKE) -C read_xrefs

slow:
	@echo "Slow tests..."
	@$(MAKE) -C c-xref
	@$(MAKE) -C emacs_c_exercise
	@$(MAKE) -C ffmpeg
	@$(MAKE) -C systemd
