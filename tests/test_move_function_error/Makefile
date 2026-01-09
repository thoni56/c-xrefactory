include ../Makefile.cli


ARGUMENTS = -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=parsing.c -rfct-target-line=47 -olcursor=4419 server.c


$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -trace -log=trace
trace: $(TEST)

EXTRA_DEBUG =
