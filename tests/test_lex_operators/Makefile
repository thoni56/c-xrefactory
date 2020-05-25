# Automated tests for checking lexing of all operators, no checks

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -create operators.c -o log

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(OK)
	@exit 0

gdb:
	gdb --args $(COMMAND)
