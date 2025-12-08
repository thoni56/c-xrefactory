include ../Makefile.boilerplate


ARGUMENTS = -fastupdate -errors -preload source.c source.preload

$(TEST):
	rm -rf CXrefs
	$(CXREF) -create . > output.tmp
	../../utils/cxref_reader >> output.tmp
	touch source.preload
	$(COMMAND) >> output.tmp
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=trace
trace: $(TEST)
