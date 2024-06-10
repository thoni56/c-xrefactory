# Automated tests for C exercise : browsing

include ../Makefile.boilerplate

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

ARGUMENTS = -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -create

CXrefs: .c-xrefrc
	@# Sources are not here but in subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(COMMAND) -o $(TEST).log

debug:
	@$(EDIT_SERVER_DRIVER) commands.input --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v .emacs | awk '{ print $$1 }'` -ex 'br main'

clean:
	rm -rf CXrefs .c-xrefrc
