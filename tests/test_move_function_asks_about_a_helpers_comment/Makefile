# Move Function asks about the comment of a helper that moves along
#
# Given a function without a comment, and a static helper only it uses, with a comment
# When it is moved, and the request does not say whether comments move
# Then the server asks, and with a yes the helper's comment moves with the helper

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
