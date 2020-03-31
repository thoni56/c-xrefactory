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

quick: simple_xrefs pragma_once process_yacc yacc_coverage cgram_coverage basic_html_generation standard_includes initializer_parsing generate_str_fill jar_reading java_example partial_jexercise edit_server_test_example push complete rename simple_extract add_parameter move_class extract_with_return


.PHONY: simple_xrefs
simple_xrefs:
	@$(MAKE) -C simple_xrefs


.PHONY: pragma_once
pragma_once:
	@$(MAKE) -C pragma_once


.PHONY: process_yacc
process_yacc:
	@$(MAKE) -C process_yacc


.PHONY: yacc_coverage
yacc_coverage:
	@$(MAKE) -C yacc_coverage


.PHONY: cgram_coverage
cgram_coverage:
	@$(MAKE) -C cgram_coverage


.PHONY: basic_html_generation
basic_html_generation:
	@$(MAKE) -C basic_html_generation


.PHONY: standard_includes
standard_includes:
	@$(MAKE) -C standard_includes


.PHONY: initializer_parsing
initializer_parsing:
	@$(MAKE) -C initializer_parsing


.PHONY: generate_str_fill
generate_str_fill:
	@$(MAKE) -C generate_str_fill


.PHONY: jar_reading
jar_reading:
	@$(MAKE) -C jar_reading


.PHONY: java_example
java_example:
	@$(MAKE) -C java_example


.PHONY: partial_jexercise
partial_jexercise:
	@$(MAKE) -C partial_jexercise


.PHONY: edit_server_test_example
edit_server_test_example:
	@$(MAKE) -C edit_server_test_example


.PHONY: push
push:
	@$(MAKE) -C push


.PHONY: complete
complete:
	@$(MAKE) -C complete


.PHONY: rename
rename:
	@$(MAKE) -C rename


.PHONY: simple_extract
simple_extract:
	@$(MAKE) -C simple_extract


.PHONY: add_parameter
add_parameter:
	@$(MAKE) -C add_parameter


.PHONY: move_class
move_class:
	@$(MAKE) -C move_class


.PHONY: extract_with_return
extract_with_return:
	@$(MAKE) -C extract_with_return

slow: emacs_c_exercise c-xref ffmpeg systemd

.PHONY: emacs_c_exercise
emacs_c_exercise:
	@$(MAKE) -C emacs_c_exercise

.PHONY: c-xref
c-xref:
	@$(MAKE) -C c-xref

.PHONY: ffmpeg
ffmpeg:
	@$(MAKE) -C ffmpeg

.PHONY: systemd
systemd:
	@$(MAKE) -C systemd
