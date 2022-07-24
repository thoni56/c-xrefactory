# Test for moving a class
#
# NOTE refactorings seems to need the CXrefs generated beforehand
# NOTE -sourcepath does not propagate, use SOURCEPATH=.

include ../Makefile.boilerplate

EDIT_SERVER_DRIVER_COMMAND=$(EDIT_SERVER_DRIVER) commands.input $(EXTRA)

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER_COMMAND) > output.tmp
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

CXrefs: .c-xrefrc
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc jexercise -sourcepath $(CURDIR)/jexercise -o CXrefs.output

clean:
	@-rm -rf CXrefs output.tmp output log *~ .c-xrefrc

gdb:
	@$(EDIT_SERVER_DRIVER_COMMAND) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br refactory'
