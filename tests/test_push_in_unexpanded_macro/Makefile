include ../Makefile.server

$(TEST):
	$(CXREF) -create > output.tmp
	@$(SERVER_DRIVER) commands.input $(EXTRA) >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-trace -log=log"
trace: $(TEST)

GDB_COMMANDS =
