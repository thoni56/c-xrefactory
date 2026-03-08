include ../Makefile.server

$(TEST):
	echo Creating
	$(CXREF_PROGRAM) -create -debug -log=trace > /dev/null
	echo Running
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
