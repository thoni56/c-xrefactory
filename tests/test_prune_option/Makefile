include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) -log=log -o output.tmp -prune pruned

$(TEST): .c-xrefrc
	$(COMMAND) > out
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
