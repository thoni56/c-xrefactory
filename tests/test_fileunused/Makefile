include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST): clean
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
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

trace: CXREF+=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output buffer

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1`
