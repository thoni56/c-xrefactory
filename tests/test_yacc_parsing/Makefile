# Automated tests with the primary purpose of adding coverage
# to the yaccparser
#
# Very simple verification of some symbols, actually that 'grep' trick
# (it only looks for the same lines in the output as in the expected
# in any order) isn't very foolproof so we should write a utility for
# checking that a symbol reference is there

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc simple_grammar.y -o $(TEST).log

# Alternate way to look for content in a file
CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@echo "Tests in '$(TEST)': $(shell tput setaf 3)No tests, just executing$(shell tput sgr0)"
	@exit 0

gdb:
	gdb --args $(COMMAND)
