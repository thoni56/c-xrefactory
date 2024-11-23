include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=14 -rfct-parameter-name="int arg" -rfct-parameter-value=42 -olcursor=70 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)
