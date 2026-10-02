include ../Makefile.server

$(TEST):
	@# Test: a Latin-1 'å' (0xE5) in a comment is one character, so the '*/'
	@# after it still ends the comment and 'x' is found on line 1
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
