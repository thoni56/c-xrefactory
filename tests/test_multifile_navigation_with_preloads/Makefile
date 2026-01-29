include ../Makefile.server

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create > /dev/null
	sleep 2
	touch common.preload1
	sleep 2
	touch common.preload2
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
