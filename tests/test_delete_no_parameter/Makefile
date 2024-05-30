include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -refactory -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -rfct-del-param -olcxparnum=1 -olcursor=36 $(CURDIR)/hello.c

$(TEST): .c-xrefrc
	$(CXREF) $(ARGUMENTS) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log .c-xrefrc

gdb:
	gdb $(CXREF) -ex 'br main' -ex 'run $(ARGUMENTS)'

lldb:
	lldb -o 'breakpoint set --name main' -o 'process launch -- $(ARGUMENTS)' $(CXREF)
