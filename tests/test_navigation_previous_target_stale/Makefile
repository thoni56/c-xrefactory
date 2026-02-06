include ../Makefile.server

# Test that PREVIOUS correctly handles navigating to a file that was modified.
# This triggers restoreToPreviousReferenceAfterRefresh() which is called when the
# TARGET file of navigation is stale.
#
# Scenario:
# - PUSH on 'counter' in source.c -> goes to header.h (definition)
# - NEXT to header.h:7 (usage in reset)
# - NEXT to source.c:4 (usage in increment)
# - PREVIOUS with header.h modified (stale) -> should refresh and go to header.h:10
#   (shifted by 3 lines in modified version)

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload
# Ensure header.modified has newer timestamp than header.preload
	sleep 2
	touch header.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
