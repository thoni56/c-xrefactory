include ../Makefile.boilerplate

ARGUMENTS = -xrefactory-II -refactory -rfct-extract-variable -olmark=44 -olcursor=49 source.c

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=log
trace: $(TEST)
