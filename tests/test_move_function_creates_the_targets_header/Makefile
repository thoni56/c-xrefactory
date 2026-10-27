# Move Function creates the target's header when there is none
#
# Given a function that main() calls, and a target.c without a target.h
# When the function is moved to target.c
# Then target.h is created with its declaration, and source.c includes it

include ../Makefile.server

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
