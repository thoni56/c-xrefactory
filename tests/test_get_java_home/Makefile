# Automated tests for olcx_push a.k.a goto-definition
#
include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) > output.tmp
	$(NO_TESTS)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc test_source -o cxrefs.log

clean:
	rm -rf .c-xrefrc CXrefs output*
