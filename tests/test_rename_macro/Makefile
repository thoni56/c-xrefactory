include ../Makefile.boilerplate

ARGUMENTS = -xrefactory-II -refactory -rfct-rename -renameto=VVV -olcursor=6790 main.c

$(TEST):
	$(COMMAND) -o output.tmp > log
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
