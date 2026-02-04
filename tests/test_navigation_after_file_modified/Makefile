include ../Makefile.server

# Test that NEXT correctly handles navigating to a file that was modified.
# This triggers restoreToNextReferenceAfterRefresh() which is called when the
# TARGET file of navigation is stale.
#
# Scenario:
# - PUSH on 'counter' in source.c
# - NEXT navigates to header.h (definition)
# - NEXT again, now source.c has a newer preload (source.modified with shifted lines)
# - Cursor stays in header.h but we send modified preload for source.c
# - Navigation targets source.c which is stale, triggering refresh
# - Reference positions should be from the MODIFIED file (line 7, not line 4)

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload1
# Ensure source.modified has newer timestamp than source.preload1
	sleep 2
	touch source.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload1 source.preload2

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
