include ../Makefile.cli

ARGUMENTS = -xrefactory-II -refactory -rfct-rename -renameto=VVV -olcursor=6790 main.c

$(TEST):
	$(COMMAND) -o output.tmp > log
	$(NORMALIZE) output.tmp | sed '/<warning/d;/<\/warning>/d' > output
	$(VERIFY)
