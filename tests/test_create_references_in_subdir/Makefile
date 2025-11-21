include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors -o output

$(TEST):
	@-$(COMMAND) > /dev/null
	ls CXrefs/subdir > output
	$(VERIFY)
