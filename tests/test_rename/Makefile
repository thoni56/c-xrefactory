include ../Makefile.boilerplate


$(TEST):
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

GDB_COMMANDS = -ex 'br refactory' -ex 'continue'
