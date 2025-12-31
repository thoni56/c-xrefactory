include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-organize-includes source.c

$(TEST): .c-xrefrc
	touch *.c
	-@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	$(VERIFY)

trace: EXTRA = -debug -log=log -lexemtrace
trace: $(TEST)
