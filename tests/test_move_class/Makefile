include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)"

$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER_COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

CXrefs: .c-xrefrc
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc test_sources -sourcepath $(CURDIR)/test_sources -o CXrefs.output

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output log

gdb:
	@$(EDIT_SERVER_DRIVER_COMMAND) 15 > output.tmp &
	sleep 1
	sudo gdb ../../src/c-xref -pid `pgrep c-xref | tail -1` -ex 'br refactoryPerformMoveClass'
