include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = xrefrc .c-xrefrc source.c -refnum=1 -o log
COMMAND = $(CXREF) $(ARGUMENTS)

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(NO_TESTS)
	@exit 0

