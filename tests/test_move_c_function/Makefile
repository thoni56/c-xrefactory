include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST):
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

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

debug gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br setAvailableRefactorings' -ex 'continue'
