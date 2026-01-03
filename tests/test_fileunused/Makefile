include ../Makefile.server


$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=log"
trace: $(TEST)

GDB_COMMANDS = -ex 'br processOptions' -ex 'br pushLocalUnusedSymbolsAction' -ex 'continue'
