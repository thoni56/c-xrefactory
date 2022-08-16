# Just running c-xref on itself to ensure that it doesn't crash

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed 's%  $(CURDIR)%  ../../src%' .c-xrefrc > .c-xrefrc.tmp
	@sed 's/=10/=1/' .c-xrefrc.tmp > .c-xrefrc
	@$(NO_TESTS)
	@../run-c-xref -p $(CURDIR) -create $(TRACE) -o c-xref.log
	@exit 0

trace: TRACE=-trace
trace: $(TEST)

clean:
	rm -rf CXrefs c-xref.log out .c-xrefrc

DEBUGGER = gdb
debug:
	$(DEBUGGER) --args ../../src/c-xref -create -xrefrc .c-xrefrc -p $(CURDIR)
