include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-add-param -olcxparnum=1 -rfct-parameter-name="int arg" -rfct-parameter-value=42 -olcursor=16 $(CURDIR)/hello.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)
