include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors $(TRACE) -refs REFS -log=log -o output.ignore

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: TRACE = -debug
trace: $(TEST)
