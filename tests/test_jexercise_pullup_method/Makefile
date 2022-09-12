# Automated tests for Java exercise: completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)
EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input $(EXTRA)

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

CXrefs: .c-xrefrc
	@# Files in the jexercise subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc

clean:
	rm -rf CXrefs .c-xrefrc

debug: DEBUGGER = gdb
debug:
	@$(EDIT_SERVER_DRIVER_COMMAND) --delay 5 > output.tmp &
	sleep 1
	sudo $(DEBUGGER) ../../src/c-xref -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br refactory'
