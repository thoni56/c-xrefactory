include ../Makefile.boilerplate


$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

GDB_COMMANDS = -ex 'br parseBufferUsingServer' -ex 'continue'
