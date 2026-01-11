include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-organize-includes -olcursor=29 source.c

$(TEST): .c-xrefrc
	touch *.c
	-@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=log -lexemtrace
trace: $(TEST)
