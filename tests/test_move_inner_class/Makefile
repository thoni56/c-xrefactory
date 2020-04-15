# Test for moving a class
#
# TODO it does not really do the move, but it prints an error message and
# exercises a lot of code, so let's keep this

include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER = ../../utils/edit_server_driver.py
EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER_COMMAND) > output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp > output
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

CXrefs: .c-xrefrc
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc test_sources -sourcepath $(CURDIR)/test_sources -o CXrefs.output

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output buffer log

gdb:
	@$(EDIT_SERVER_DRIVER_COMMAND) 15 > output.tmp &
	sleep 1
	sudo gdb ../../src/c-xref -pid `pgrep c-xref | tail -1` -ex 'br refactoryPerformMoveClass'
