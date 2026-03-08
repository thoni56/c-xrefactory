include ../Makefile.cli

ARGUMENTS = initializer_parsing.c -create -o output.tmp

$(TEST): .c-xrefrc
	@$(COMMAND)
	@../../utils/cxref_extract CXrefs E0 >> output.tmp
	$(NORMALIZE) output.tmp > output
	@$(VERIFY)
