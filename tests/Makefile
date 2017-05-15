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
	@Echo "Quick tests..."
	@make -C pragma_once
	@make -C process_yacc
	@make -C basic_html_generation
	@make -C standard_includes

slow:
	@echo "Slow tests..."
	@make -C ffmpeg
	@make -C systemd

