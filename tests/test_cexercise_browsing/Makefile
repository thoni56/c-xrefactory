# Automated tests for C exercise : browsing

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXREF = ../../src/c-xref
COMMAND = $(CXREF) -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -create

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br main'

clean:
	rm -rf CXrefs .c-xrefrc
