include ../Makefile.boilerplate

# This test token pasting which mostly works. It does not include token pasting into
# numbers, which doesn't. There is another, suspended, test for that.

ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)

EXTRA_DEBUG =
