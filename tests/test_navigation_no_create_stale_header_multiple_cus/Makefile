include ../Makefile.server

# Scenario: Navigation without initial project scan (-create)
#
# Given NO initial project scan has been run
# And common.h is in an editor buffer and has been modified (+3 lines)
# And a.c, b.c, c.c all include common.h
# When the user PUSHes on 'counter' in a.c and NEXTs through all references
# And common.h becomes stale on the last NEXT
# Then all CUs are discovered and parsed on-demand
# And navigation visits references in all three CUs
# And navigation wraps to common.h with shifted positions
#
# This is a specification for ADR 22 (lightweight file structure scanning).
# When this test passes, on-demand parsing replaces the full project scan.

$(TEST):
# No -create: the server must discover and parse CUs on-demand
# Create preloads for original content
	cp a.c a.preload
	cp common.h common.preload
# Ensure common.modified has newer timestamp than common.preload
	sleep 2
	touch common.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f a.preload common.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
