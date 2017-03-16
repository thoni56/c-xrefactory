# Run all tests
ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif


all: quick slow

quick:
	@Echo "Quick tests..."
	@make -C pragma_once
	@make -C basic_html_generation
	@make -C standard_includes

slow:
	@echo "Slow tests..."
	@make -C ffmpeg
	@make -C systemd

