include ../Makefile.cli


ARGUMENTS = -errors source.c

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -debug -log=trace -yydebug
trace: $(TEST)

EXTRA_DEBUG =
