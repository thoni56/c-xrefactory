include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -create -log=log -o out

$(TEST):
	rm -rf output $@.log CXrefs Xrefs
	$(COMMAND)
	$(NO_TESTS)
	exit 0

gdb:
	gdb --args $(COMMAND)
