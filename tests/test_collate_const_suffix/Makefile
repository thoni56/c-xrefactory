include ../Makefile.boilerplate

$(TEST):
	$(COMMAND) source.c -o output.tmp
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader CXrefs >> output
	$(VERIFY)
