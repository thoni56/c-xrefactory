include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) source.c -log=log -o output.tmp $(EXTRAS)

$(TEST): .c-xrefrc
	$(COMMAND) > out
	@$(NORMALIZE) output.tmp > output
	$(NO_TESTS)

trace: TRACE = -trace
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp
