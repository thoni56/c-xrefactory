# Automated tests for C exercise : completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
	@$(NORMALIZE) output.tmp > output
	@cmp output expected ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

CXrefs: .c-xrefrc
	@sed -e 's:CXrefs$$:CXrefs/cexercise:' .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc
