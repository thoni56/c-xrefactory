include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input $(EXTRA)

$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER_COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

CXrefs: .c-xrefrc
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc test_sources -sourcepath $(CURDIR)/test_sources -o CXrefs.output

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output log

debug: DEBUGGER = gdb
debug:
	@$(EDIT_SERVER_DRIVER_COMMAND) --delay 5 > output.tmp &
	sleep 1
	sudo $(DEBUGGER) ../../src/c-xref -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br performMoveClass'
