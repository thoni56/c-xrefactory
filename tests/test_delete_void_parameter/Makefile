include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-del-param -olcxparnum=1 -olcursor=17 $(CURDIR)/hello.c

$(TEST): .c-xrefrc
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA=-debug
trace: $(TEST)
