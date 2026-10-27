# Move Function asks whether to move the comment
#
# Given a function with a comment above it
# When it is moved, and the request does not say whether comments move
# Then the server asks, and with a yes the comment moves with it

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
