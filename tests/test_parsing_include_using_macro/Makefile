include ../Makefile.cli


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) source.c -log=log -o output.tmp $(EXTRAS)

$(TEST): .c-xrefrc
	$(COMMAND) > /dev/null
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: TRACE = -debug
trace: $(TEST)
