include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)

$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

clean:
	rm -rf CXrefs .c-xrefrc

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br parseBufferUsingServer' -ex 'continue'
