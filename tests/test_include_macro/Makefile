include ../Makefile.boilerplate


COMMAND = ../../src/c-xref
ARGUMENTS = -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) source.c -log=log -o output.tmp $(EXTRAS)

$(TEST): .c-xrefrc
	$(COMMAND) $(ARGUMENTS) > out
	@$(NORMALIZE) output.tmp > output
	$(NO_TESTS)

trace: TRACE = -trace
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

DEBUGGER = gdb
debug: TRACE = -trace
debug:
	$(DEBUGGER) $(COMMAND) -ex 'br main' -ex 'run $(ARGUMENTS)'
