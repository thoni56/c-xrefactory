include ../Makefile.boilerplate

ARGUMENTS = initializer_parsing.c -create -o output

$(TEST): .c-xrefrc
	@$(COMMAND)
	@../../utils/cxref_extract E0 > output
	@$(VERIFY)
