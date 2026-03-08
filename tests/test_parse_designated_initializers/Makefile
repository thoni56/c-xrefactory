include ../Makefile.cli

ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)
