include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -no-stdoptions -xrefactory-II -refactory -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -rfct-del-param -olcxparnum=1 -olcursor=36 $(CURDIR)/hello.c

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
