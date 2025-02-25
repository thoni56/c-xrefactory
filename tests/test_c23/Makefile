include ../Makefile.boilerplate


ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) 2>&1 | grep -v "active project" > output || :
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)

EXTRA_DEBUG =
