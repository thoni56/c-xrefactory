include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
	fi

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc test_source -o $@.log

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'`
