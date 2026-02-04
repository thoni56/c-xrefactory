include ../Makefile.server

# Test that NEXT correctly handles navigating to a file that was modified.
# This triggers restoreToNextReferenceAfterRefresh() which is called when the
# TARGET file of navigation is stale.
#
# Scenario:
# - PUSH on 'counter' in header.h (at definition)
# - NEXT navigates to source.c (which has a newer preload, simulating modification)
# - The target file (source.c) is stale, triggering refresh during navigation

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads - header.h first (used for PUSH)
	cp header.h header.preload
	cp source.c source.preload1
# Ensure timestamp difference
	sleep 2
# Create newer preload for source.c (simulating modification after PUSH)
	touch source.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload1 source.preload2

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
