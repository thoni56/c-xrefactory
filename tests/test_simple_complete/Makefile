# Automated tests for exercising completion functionality

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer > output.tmp
	@$(NORMALIZE) output.tmp > output
	@cmp output expected ; \
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
	@$(CXREF) test_source -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" buffer 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1`
