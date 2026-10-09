include ../Makefile.server


$(TEST):
	@rm -rf .c-xref
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS = -ex 'br refactory' -ex 'continue'
