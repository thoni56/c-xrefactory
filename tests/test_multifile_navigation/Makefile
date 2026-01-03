include ../Makefile.server

$(TEST):
	$(CXREF) -create
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra \'-trace\'
trace: $(TEST)

GDB_COMMANDS =
