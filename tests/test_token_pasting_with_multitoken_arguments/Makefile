include ../Makefile.boilerplate


ARGUMENTS = -errors source.c

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -trace -log=trace
trace: $(TEST)

EXTRA_DEBUG =
