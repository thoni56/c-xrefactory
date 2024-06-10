include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=1 -rfct-param1="int arg" -rfct-param2=42 -olcursor=16 $(CURDIR)/hello.c

$(TEST): clean
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log
