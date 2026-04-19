include ../Makefile.server


$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

xGDB_COMMANDS = -ex 'br serverPerformRefactoring' -ex 'continue'
