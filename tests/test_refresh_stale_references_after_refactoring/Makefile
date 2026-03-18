include ../Makefile.server

# Scenario: Navigation finds correct position after move-function
#
# Given source_a.c defines target() at line 1 and other() at line 5
# And source_b.c calls target()
# When target() is moved below other() in source_a.c
# And we navigate to the definition of target() from source_b.c
# Then we should land on target()'s new position, not the old one

$(TEST):
	@-rm -rf .c-xref/db
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
