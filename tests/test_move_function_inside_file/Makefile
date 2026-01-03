include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=source.c -rfct-target-line=4 -olcursor=94 source.c

$(TEST): .c-xrefrc
	touch *.c
	@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)
