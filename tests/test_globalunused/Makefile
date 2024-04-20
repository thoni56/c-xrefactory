include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "diff output expected" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: EXTRA = \'-trace\'
trace: $(TEST)

COMMAND = $(CXREF) -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -create

CXrefs: .c-xrefrc
	@$(COMMAND) -o $(TEST).log

clean:
	@-rm -rf CXrefs output.log output.tmp output


DEBUGGER = gdb
debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo $(DEBUGGER) -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br scanForGlobalUnused' -ex 'br setSelectedVisibleItems' -ex 'continue'
