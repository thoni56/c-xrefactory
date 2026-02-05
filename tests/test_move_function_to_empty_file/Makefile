include ../Makefile.cli


ARGUMENTS = -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=target.c -rfct-target-line=2 -olcursor=4419 server.c


$(TEST):
	rm -rf CXrefs
	$(COMMAND) | grep -v warning > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -trace -log=trace
trace: $(TEST)

EXTRA_DEBUG =
