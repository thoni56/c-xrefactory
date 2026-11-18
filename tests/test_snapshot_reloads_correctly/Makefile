include ../Makefile.server

# Scenario: db correct after snapshot load and reparse of stale file
#
# Given a legacy project (explicit source dir)
# When session 1 navigates and exits (saves snapshot)
# And a source file is touched (making it stale)
# And session 2 starts, loads snapshot, pushes on target, which reparses the stale CU,
#   and exits
# Then the saved db should contain correct references for all symbols

$(TEST):
	@$(SERVER_DRIVER) $(EXTRA) session1.input > /dev/null 2>&1
	@sleep 2 && touch other.c
	@$(SERVER_DRIVER) $(EXTRA) session2.input > /dev/null 2>&1
	@../../utils/cxref_reader .c-xref/db > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
