# Automated tests for Java exercise: completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) > output.tmp
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
	# Files in the jexercise subdir and the project name
	sed -ibak -e s:CXrefs$$:CXrefs/jexercise: .c-xrefrc -e s:$(CURDIR)]$$:$(CURDIR)/jexercise]: -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc
	@$(CXREF) -create  -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc
