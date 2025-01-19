include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST):
	$(EDIT_SERVER_DRIVER) --cxref ../../src/c-xref commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

GDB_COMMANDS = -ex 'br olcxReferenceGotoCompletion' -ex 'continue'
