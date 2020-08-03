# Automated tests for exercising completion functionality

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: CXREF+=-trace
trace: $(TEST)

CXrefs:
	@$(CXREF) test_source -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1`
