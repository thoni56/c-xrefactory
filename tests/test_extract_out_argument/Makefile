include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -refactory -rfct-extract-function -olmark=50 -olcursor=80 $(CURDIR)/source.c

$(TEST): clean
	$(CXREF) $(ARGUMENTS) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log

debug: DEBUGGER=gdb
debug:
	$(DEBUGGER) $(CXREF) -ex 'br main' -ex 'run $(ARGUMENTS)'
