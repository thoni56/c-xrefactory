include ../Makefile.boilerplate


ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -debug
trace: $(TEST)

EXTRA_DEBUG =
