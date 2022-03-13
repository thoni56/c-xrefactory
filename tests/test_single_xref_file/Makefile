include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int*.c -refnum=1 -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(NO_TESTS)
	@exit 0

debug gdb:
	gdb --args $(COMMAND)
