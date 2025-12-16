# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

ARGUMENTS = -create $(CURDIR)/../../src/main.c -errors -filetrace $(TRACE) -o c-xref.log

$(TEST): .c-xrefrc
	@# Point sources to the src directory
	sed 's:/tests/test_c-xref/../..::' .c-xrefrc > .c-xrefrc.new
	mv .c-xrefrc.new .c-xrefrc
	@../run-with-progress $(COMMAND) || exit
	-grep ERROR run.stderr > output.tmp
	cat output.tmp | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: TRACE=-debug -log=trace
trace: $(TEST)
