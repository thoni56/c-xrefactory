# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc -refnum=1 .c-xrefrc single_int*.c -o log

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(NO_TESTS)
	@exit 0

clean:
	rm -rf CXrefs .c-xrefrc
gdb:
	gdb --args $(COMMAND)
