include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=source.c -rfct-target-line=4 -olcursor=94 source.c

$(TEST): .c-xrefrc
	@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)
