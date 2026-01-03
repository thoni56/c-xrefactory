include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=2 -rfct-parameter-name=int\ arg -rfct-parameter-value=42 -olcursor=17 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-debug -log=log
trace: $(TEST)
