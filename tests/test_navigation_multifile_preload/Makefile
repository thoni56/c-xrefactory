include ../Makefile.server

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
	sleep 2
	touch source1.preload common.preload source2.preload
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
