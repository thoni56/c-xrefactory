include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-del-param -olcxparnum=1 -olcursor=36 $(CURDIR)/hello.c

$(TEST): .c-xrefrc
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-debug
trace: $(TEST)
