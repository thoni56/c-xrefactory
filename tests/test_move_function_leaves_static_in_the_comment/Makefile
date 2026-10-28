# Move Function leaves 'static' in the comment
#
# Given a static function with a comment above it that says static
# When it is moved, with its comment, to another file
# Then 'static' is removed from the definition, not from the comment

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
