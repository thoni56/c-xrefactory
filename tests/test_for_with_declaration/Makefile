include ../Makefile.boilerplate


CXREF = ../../src/c-xref
COMMAND = $(CXREF) -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) source.c -log=log -o output.tmp

$(TEST): .c-xrefrc
	$(COMMAND)
	@$(NORMALIZE) output.tmp > output
	grep "error len" output ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

DEBUGGER = gdb
debug: TRACE = -trace
debug:
	$(DEBUGGER) --args $(COMMAND)
