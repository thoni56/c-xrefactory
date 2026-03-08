include ../Makefile.cli


ARGUMENTS =

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -debug
trace: $(TEST)

EXTRA_DEBUG =
