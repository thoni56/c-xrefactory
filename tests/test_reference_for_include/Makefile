include ../Makefile.cli

$(TEST):
	$(COMMAND) -create source.c > output.tmp
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
