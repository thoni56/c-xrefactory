include ../Makefile.cli


ARGUMENTS = -xrefactory-II -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=target.c -rfct-target-line=3 -olcursor=32 source.c

$(TEST): .c-xrefrc
	touch *.[ch]
	-@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	$(VERIFY)

trace: EXTRA = -debug -log=log -lexemtrace
trace: $(TEST)
