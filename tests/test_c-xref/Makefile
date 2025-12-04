# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

ARGUMENTS = -create -errors -filetrace $(TRACE) -o c-xref.log

$(TEST): .c-xrefrc
	sed -i 's:/tests/test_c-xref/../..::' .c-xrefrc
	@../run-with-progress $(COMMAND) || exit
	-grep ERROR run.stderr > output.tmp
	cat output.tmp | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: TRACE=-debug -log=trace
trace: $(TEST)
