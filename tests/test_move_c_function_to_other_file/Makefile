include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=target.c -rfct-param1=3 -olcursor=32 source.c

$(TEST): .c-xrefrc
	@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)
