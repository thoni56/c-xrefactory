include ../Makefile.boilerplate

ARGS = -xrefrc .c-xrefrc -xrefactory-II -p $(CURDIR) -refactory -rfct-rename -renameto=VVV -olcursor=6790 main.c

$(TEST):
	../../src/c-xref $(ARGS) -o output.tmp > log
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

debug:
	gdb --args ../../src/c-xref $(ARGS)
