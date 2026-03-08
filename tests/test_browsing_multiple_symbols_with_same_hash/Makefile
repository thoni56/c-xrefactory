include ../Makefile.server

# Test that stale file refresh preserves references from other files.
# Uses actual c-xrefactory source files with X-macro pattern (OLO_COMPLETION_FORWARD).
# This tests the fix for the bug where the merge loop only processed
# the first item at each hash slot, missing items in the linked list.
#
# Scenario:
# - PUSH on 'OLO_COMPLETION_FORWARD' in server.h (X-macro definition)
# - NEXT to cxref.c
# - NEXT -> should reach options.c

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads with progressively newer timestamps to trigger staleness on each visit
# (simulates emacs creating fresh preloads each time a buffer is visited)
	sleep 1
	cp server.h server.preload1
	cp cxref.c cxref.preload
	cp options.c options.preload
	sleep 1
	cp server.h server.preload2
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp | sed '/<warning/d;/<\/warning>/d' > output
	$(VERIFY)

local-clean:
	rm -f *.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
