include ../Makefile.boilerplate

$(TEST):
	touch source.preload # ensure the preloaded buffer has a more recent timestamp
	$(CXREF) -create > /dev/null 2>&1
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=log"
trace: $(TEST)

GDB_COMMANDS =
