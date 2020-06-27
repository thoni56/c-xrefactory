# Automated tests to ensure that a full update does not give unexpected errors in interaction
#
include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer >output.tmp 2>&1
	@sed -e "s/len=[0-9]*/len=<n>/g" -e "s#$(CURDIR)#CURDIR#g" output.tmp > output
	@if grep -q "ERROR.*commons.c:" output > /dev/null ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc test_source -o cxrefs.log

clean:
	rm -rf .c-xrefrc CXrefs output* buffer *.log

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1` -ex 'br main' -ex 'continue'
