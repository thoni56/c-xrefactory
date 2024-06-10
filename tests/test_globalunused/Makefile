include ../Makefile.boilerplate


$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

ARGUMENTS = -create

CXrefs: .c-xrefrc
	@$(COMMAND) -o $(TEST).log

clean:
	@-rm -rf CXrefs output.log output.tmp output


DEBUGGER = gdb
debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo $(DEBUGGER) -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br scanForGlobalUnused' -ex 'br setSelectedVisibleItems' -ex 'continue'
