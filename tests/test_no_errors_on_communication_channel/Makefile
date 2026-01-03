# Automated tests to ensure that a full update does not give unexpected errors in server
#
include ../Makefile.boilerplate


$(TEST): CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) >output.tmp1 2>&1
	$(NORMALIZE) output.tmp1 > output.tmp
	@-grep -q "ERROR.*commons.c:" output.tmp > output
	rm output.tmp1
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF) test_source -o cxrefs.log

GDB_COMMANDS = -ex 'br main' -ex 'continue'
