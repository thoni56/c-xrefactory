include ../Makefile.server

$(TEST):
	@# Test: a file under a -prune'd directory is not part of the project
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
