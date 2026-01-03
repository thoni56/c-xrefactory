include ../Makefile.server

CXREF = ../../src/c-xref

$(TEST):
	$(SERVER_DRIVER) --cxref ../../src/c-xref commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

GDB_COMMANDS = -ex 'br olcxReferenceGotoCompletion' -ex 'continue'
