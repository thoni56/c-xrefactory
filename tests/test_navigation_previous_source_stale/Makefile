include ../Makefile.server

# Test that PREVIOUS correctly handles when the SOURCE file (where cursor is) is stale.
# This triggers the source file refresh path in gotoPreviousReference() (lines 1089-1094).
#
# Scenario:
# - PUSH on 'counter' in source.c -> goes to header.h (definition)
# - NEXT to header.h:7 (usage in reset)
# - NEXT to source.c:4 (usage in increment)
# - PREVIOUS with source.c modified (stale) -> should refresh source.c first,
#   restore to nearest reference, then navigate to previous

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload
# Ensure source.modified has newer timestamp than source.preload
	sleep 2
	touch source.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
