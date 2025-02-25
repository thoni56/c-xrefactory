include ../Makefile.boilerplate


ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) 2>&1 > output.tmp
	$(NORMALIZE) output.tmp > output2
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)

EXTRA_DEBUG =
