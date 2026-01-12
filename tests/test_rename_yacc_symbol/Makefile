include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace -log=trace'
trace: $(TEST)

GDB_COMMANDS =
