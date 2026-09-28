include ../Makefile.server

$(TEST):
	@touch source.c.organized   # a real client's preload is newer than the file
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
