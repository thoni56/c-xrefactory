include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -refactory -rfct-add-param -olcxparnum=14 -rfct-param1="int arg" -rfct-param2=42 -olcursor=70 $(CURDIR)/source.c

$(TEST): clean
	$(CXREF) $(ARGUMENTS) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log
