include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors -o output.tmp src

$(TEST):
	$(COMMAND) | grep -v progress > log
	../../utils/cxref_extract CXrefs referenceItem.vApplClass > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
