include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-extract-function -olmark=93 -olcursor=114 $(CURDIR)/source.c

$(TEST):
	$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-debug
trace: $(TEST)
