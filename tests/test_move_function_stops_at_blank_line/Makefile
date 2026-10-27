# Move Function stops at a blank line when it takes the comment along
#
# Given a comment, a blank line and then the function's own comment
# When the function is moved with its comments
# Then only its own comment moves

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
