# Automated tests for C exercise : completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: CXREF+=-trace
trace: $(TEST)

CXrefs: .c-xrefrc
	@# Sources in subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc
