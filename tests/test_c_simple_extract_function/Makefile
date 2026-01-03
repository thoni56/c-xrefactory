include ../Makefile.server


$(TEST):
	@# First do the extract
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

GDB_COMMANDS = -ex 'br refactory' -ex 'continue'
