include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace -log=debug'
trace: $(TEST)
