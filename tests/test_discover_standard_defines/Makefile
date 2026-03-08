include ../Makefile.cli


ARGUMENTS = -create

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS += -debug -log=trace
trace: $(TEST)

EXTRA_DEBUG =
