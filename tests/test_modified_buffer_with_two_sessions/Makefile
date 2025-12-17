include ../Makefile.boilerplate

$(TEST):
	touch source.preload # ensure the preloaded buffer has a more recent timestamp
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=log"
trace: $(TEST)

GDB_COMMANDS =
