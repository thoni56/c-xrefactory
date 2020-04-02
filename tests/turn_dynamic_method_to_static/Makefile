# Test for moving a class
#
# NOTE refactorings seems to need the CXrefs generated beforehand
# NOTE -sourcepath does not propagate, use SOURCEPATH=.

include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER = ../../utils/edit_server_driver.py
EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer

$(TEST): CXrefs
	@-SOURCEPATH=. $(EDIT_SERVER_DRIVER_COMMAND) > output.tmp
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

trace: CXREF+=-trace
trace: $(TEST)

CXrefs:
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc jexercise -sourcepath $(CURDIR)/jexercise -o CXrefs.output

clean:
	@-rm -rf CXrefs output.tmp output buffer log

gdb:
	@$(EDIT_SERVER_DRIVER_COMMAND) 15 > output.tmp &
	sleep 1
	sudo gdb ../../src/c-xref -pid `pgrep c-xref | tail -1` -ex 'br refactoryTurnDynamicToStatic'
