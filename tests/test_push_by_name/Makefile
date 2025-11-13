include ../Makefile.boilerplate

$(TEST):
	$(CXREF) -create -o out
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

GDB_COMMANDS =
