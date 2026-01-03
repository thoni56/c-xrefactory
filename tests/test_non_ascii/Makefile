include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-rename -renameto=f22 -olcursor=131 source.c

$(TEST):
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -debug
trace: $(TEST)
