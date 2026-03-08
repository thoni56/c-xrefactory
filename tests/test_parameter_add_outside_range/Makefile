include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=14 -rfct-parameter-name="int arg" -rfct-parameter-value=42 -olcursor=70 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA=-debug -log=log
trace: $(TEST)
