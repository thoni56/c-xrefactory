# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

ARGUMENTS = -create -errors $(TRACE) -o c-xref.log -filetrace

$(TEST): .c-xrefrc
	$(COMMAND) >output 2>&1
	$(VERIFY)

trace: TRACE=-debug -log=trace
trace: $(TEST)
