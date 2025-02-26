include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) source.c -log=log

$(TEST): .c-xrefrc
	$(COMMAND) 2>&1 > output.tmp
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: TRACE = -trace
trace: $(TEST)
