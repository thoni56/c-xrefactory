include ../Makefile.boilerplate

$(TEST):
	$(COMMAND) -create source.c > output.tmp
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)
