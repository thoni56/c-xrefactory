include ../Makefile.server


$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA=--extra "-debug"
trace: $(TEST)

GDB_COMMANDS = -ex 'br server' -ex 'continue'
