include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

clean:
	rm -rf CXrefs .c-xrefrc

debug gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br refactory' -ex 'continue'
