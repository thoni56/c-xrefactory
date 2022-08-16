# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed 's%  $(CURDIR)%  ../../src%' .c-xrefrc > .c-xrefrc.tmp
	@sed 's/=10/=1/' .c-xrefrc.tmp > .c-xrefrc
	@../run-c-xref -p $(CURDIR) -create $(TRACE) -o c-xref.log
	@cp output output.tmp
	@grep '<progress>' output.tmp > output
	@$(VERIFY)
	@exit 0

trace: TRACE=-trace
trace: $(TEST)

clean:
	rm -rf CXrefs c-xref.log out .c-xrefrc

DEBUGGER = gdb
debug:
	$(DEBUGGER) --args ../../src/c-xref -create -xrefrc .c-xrefrc -p $(CURDIR)
