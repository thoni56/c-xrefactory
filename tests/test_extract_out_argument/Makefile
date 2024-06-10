include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -refactory -rfct-extract-function -olmark=50 -olcursor=80 $(CURDIR)/source.c

$(TEST): clean
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log
