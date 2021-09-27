# Automated tests for olcx_push a.k.a goto-definition
#
include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc test_source -o cxrefs.log

clean:
	rm -rf .c-xrefrc CXrefs output*

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -ex 'br mainAnswerEditAction' -pid `pgrep c-xref | tail -1` -ex 'continue'
