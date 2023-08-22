# Automated tests with the primary purpose of adding coverage
# to the yaccparser
#

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc simple_grammar.y -o $(TEST).log

$(TEST):
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@echo "Tests in '$(TEST)': $(shell tput setaf 3)No tests, just executing$(shell tput sgr0)"
	@exit 0

debug: DEBUGGER=gdb
debug:
	$(DEBUGGER) --args $(COMMAND)
