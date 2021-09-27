# Automated tests for C exercise : browsing

include ../Makefile.boilerplate

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXREF = ../../src/c-xref
COMMAND = $(CXREF) -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -create

CXrefs: .c-xrefrc
	@# Sources in subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(COMMAND) -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc
