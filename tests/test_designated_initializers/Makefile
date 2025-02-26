include ../Makefile.boilerplate

ARGUMENTS = source.c -errors

$(TEST):
	$(COMMAND) 2>&1 > output.tmp
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)
