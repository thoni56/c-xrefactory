include ../Makefile.boilerplate


ARGUMENTS = -fastupdate -errors -preload source.c source.preload

$(TEST): clean_cxrefs
	../../utils/cxref_reader > output.tmp # should be empty since source.c is empty
	$(COMMAND) >> output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=trace
trace: $(TEST)

clean_cxrefs: .c-xrefrc
	rm -rf CXrefs
	$(CXREF) -create . -o /dev/null
