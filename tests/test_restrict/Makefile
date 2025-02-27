include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) source.c -log=log

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: TRACE = -trace
trace: $(TEST)
