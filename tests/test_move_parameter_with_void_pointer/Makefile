include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-move-param -olcxparnum=1 -olcxparnum2=2 -olcursor=17 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-debug
trace: $(TEST)
