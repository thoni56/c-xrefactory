include ../Makefile.server

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create > /dev/null
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
