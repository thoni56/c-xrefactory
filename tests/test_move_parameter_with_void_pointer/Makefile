include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-move-param -olcxparnum=1 -olcxparnum2=2 -olcursor=17 $(CURDIR)/source.c

$(TEST): clean
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log
