include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
