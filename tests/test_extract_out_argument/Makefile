include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-extract-function -olmark=50 -olcursor=80 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-debug
trace: $(TEST)
