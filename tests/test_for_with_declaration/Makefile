include ../Makefile.boilerplate

ARGUMENTS = -errors

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
