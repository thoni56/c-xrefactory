include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-extract-variable -olmark=44 -olcursor=50 $(CURDIR)/source.c

$(TEST): clean
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output log
