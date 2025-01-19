include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST):
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

GDB_COMMANDS = -ex 'br initLogging' -ex continue
