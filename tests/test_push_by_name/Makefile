include ../Makefile.boilerplate

$(TEST):
	$(CXREF) -create
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

GDB_COMMANDS =
