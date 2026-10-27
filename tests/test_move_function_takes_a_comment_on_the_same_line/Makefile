# Move Function takes a comment on the same line as the end of the function
#
# Given a function with a comment after its closing brace
# When it is moved without comments
# Then the comment on the same line moves with it

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
