# Automated tests for checking refactoring "Delete argument"
#
# This test does not currently work, the communication between emacs and the server
# has not been sufficiently explored, the current commands does not reflect the correct
# interactions

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)

$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

gdb:
	gdb --args $(COMMAND)

clean:
	rm -rf CXrefs .c-xrefrc

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br getActualMacroArguments'  -ex 'continue'
