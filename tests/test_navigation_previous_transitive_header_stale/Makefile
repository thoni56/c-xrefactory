include ../Makefile.server

# Test that PREVIOUS correctly handles navigating to a transitively included
# header that was modified. The stale header (inner.h) is NOT directly included
# by the CU (source.c) — it's included via middle.h.
#
# Scenario:
# - PUSH on 'counter' in source.c -> goes to inner.h:4 (definition)
# - NEXT to inner.h:7 (usage in reset)
# - NEXT to source.c:4 (usage in increment)
# - PREVIOUS with inner.h modified (stale, +3 lines) -> should refresh and
#   go to inner.h:10 (shifted from line 7)

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp inner.h inner.preload
	cp source.c source.preload
# Ensure inner.modified has newer timestamp than inner.preload
	sleep 2
	touch inner.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f inner.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
