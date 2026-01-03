include ../Makefile.cli


ARGUMENTS = -xrefactory-II -create -errors $(EXTRA) source.c -log=log

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader >> output
	$(VERIFY)

trace: EXTRA = -debug
trace: $(TEST)
