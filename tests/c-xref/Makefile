# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

ARGUMENTS = -create -errors -filetrace $(TRACE) -o c-xref.log

$(TEST): .c-xrefrc
	@../run-with-progress $(COMMAND) || exit -1

original: CXREF = ../../src/c-xref.orig
original: $(TEST)

trace: TRACE=-debug -log=trace
trace: $(TEST)
