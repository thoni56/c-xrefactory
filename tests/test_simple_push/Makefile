# Automated tests for olcx_push a.k.a goto-definition
#
include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc
EDIT_SERVER_DRIVER = ../../utils/edit_server_driver.py

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
	@sed -e "s/len=[0-9]*/len=<n>/g" -e "s#$(CURDIR)#CURDIR#g" output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		diff output expected ; \
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
	rm -rf .c-xrefrc CXrefs output* buffer

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer 5 > output.tmp &
	sleep 1
	sudo gdb -ex 'br mainAnswerEditAction' -pid `pgrep c-xref | tail -1` -ex 'continue'
