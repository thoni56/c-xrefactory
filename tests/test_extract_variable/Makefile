include ../Makefile.cli

ARGUMENTS = -xrefactory-II -refactory -rfct-extract-variable -olmark=49 -olcursor=44 source.c

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=log
trace: $(TEST)
