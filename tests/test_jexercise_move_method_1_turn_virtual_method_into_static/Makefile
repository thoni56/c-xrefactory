# Automated tests for Java exercise: completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

CXrefs: .c-xrefrc
	@# Files in the jexercise subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc
