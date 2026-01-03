include ../Makefile.cli

$(TEST):
	$(COMMAND) source.c -o output.tmp
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)
