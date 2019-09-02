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
	@make -C simple_xrefs
	@make -C pragma_once
	@make -C process_yacc
	@make -C basic_html_generation
	@make -C standard_includes
	@make -C initializer_parsing
	@make -C generate_str_fill
	@make -C read_xrefs

slow:
	@echo "Slow tests..."
	@make -C c-xref
	@make -C emacs_c_exercise
	@make -C ffmpeg
	@make -C systemd
