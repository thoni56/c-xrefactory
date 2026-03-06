include ../Makefile.server

# Scenario: Snapshot does not preserve stale preloaded positions
#
# Given session 1 navigates with a preloaded (unsaved) header.h
#   where the declaration is shifted to line 3
# And session 1 saves a snapshot on exit
# When session 2 starts from the snapshot WITHOUT the preload
# Then navigation should show the disk position (line 2), not
#   the preloaded position (line 3)

$(TEST):
	@rm -rf .c-xref
	@$(SERVER_DRIVER) $(EXTRA) session1.input > output.tmp
	@$(SERVER_DRIVER) $(EXTRA) session2.input >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
