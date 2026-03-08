include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=2 -rfct-parameter-name="int arg" -rfct-parameter-value=42 -olcursor=16 $(CURDIR)/hello.c

$(TEST):
	$(COMMAND) $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA=-debug -log=log
trace: $(TEST)
