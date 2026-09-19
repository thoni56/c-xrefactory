include ../Makefile.cli


ARGUMENTS = -xref -fastupdate -errors -preload source.c source.preload

$(TEST):
	rm -rf CXrefs
	$(CXREF) -xref -create . > output.tmp
	../../utils/cxref_reader >> output.tmp
	touch source.preload
	$(COMMAND) >> output.tmp
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=trace
trace: $(TEST)
