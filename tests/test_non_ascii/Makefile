include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-rename -renameto=f22 -olcursor=131 -encoding=utf source.c

$(TEST):
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)
