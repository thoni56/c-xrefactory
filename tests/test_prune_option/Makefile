include ../Makefile.boilerplate


COMMAND = ../../src/c-xref
ARGUMENTS = -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) -log=log -o output.tmp -prune pruned

$(TEST): .c-xrefrc
	$(COMMAND) $(ARGUMENTS) > out
	@$(NORMALIZE) output.tmp > output
	grep "pruned_source.c" output ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

trace: TRACE = -trace
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

DEBUGGER = gdb
debug: TRACE = -trace
debug:
	$(DEBUGGER) $(COMMAND) -ex 'br main' -ex 'run $(ARGUMENTS)'
