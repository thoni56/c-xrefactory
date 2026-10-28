# Move Function leaves the comment out of the extern
#
# Given a static function with a comment above it
# When it is moved, with its comment, to a file that has a header
# Then the extern declared in that header has no comment in it

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
