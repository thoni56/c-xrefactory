# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

ARGUMENTS = -create -errors $(TRACE) -o c-xref.log -log=log

$(TEST): .c-xrefrc
	@$(COMMAND)
	@../../utils/cxref_reader > output
	$(VERIFY)

trace: TRACE=-debug -log=trace
trace: $(TEST)
