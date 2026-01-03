include ../Makefile.cli


ARGUMENTS = -update -errors source.c

$(TEST):
	-rm -rf CXrefs
	$(COMMAND) > output.tmp
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=trace
trace: $(TEST)
