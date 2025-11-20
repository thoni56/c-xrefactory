include ../Makefile.boilerplate

# There should be 27 references to vApplClass so one line with the reference item itself
# gives 28 in total

ARGUMENTS = -xrefactory-II -create -errors -o output.tmp src

$(TEST):
	$(COMMAND) | grep -v progress > log
	../../utils/cxref_extract referenceItem.vApplClass > output
	$(VERIFY)
