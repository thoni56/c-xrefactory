include ../Makefile.boilerplate


$(TEST):
	@# First do the extract
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

GDB_COMMANDS = -ex 'br refactory' -ex 'continue'
