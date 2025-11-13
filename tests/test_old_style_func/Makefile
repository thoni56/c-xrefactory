include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) source.c -log=log -o output.tmp

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	grep "error len" output ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

trace: TRACE = -debug
trace: $(TEST)
