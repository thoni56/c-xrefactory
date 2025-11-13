include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) source.c -log=log -o output.tmp $(EXTRAS)

$(TEST): .c-xrefrc
	$(COMMAND) > out
	@$(NORMALIZE) output.tmp > output
	$(NO_TESTS)

trace: TRACE = -debug
trace: $(TEST)
