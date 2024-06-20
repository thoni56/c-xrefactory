# Automated tests to ensure that a full update does not give unexpected errors in interaction
#
include ../Makefile.boilerplate


$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) >output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	@if grep -q "ERROR.*commons.c:" output > /dev/null ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF) test_source -o cxrefs.log

clean:
	rm -rf .c-xrefrc CXrefs output* *.log

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br main' -ex 'continue'
