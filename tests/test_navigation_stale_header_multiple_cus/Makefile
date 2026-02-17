include ../Makefile.server

# Scenario: Stale header included by multiple CUs
#
# Given common.h is in an editor buffer and has been modified (+3 lines)
# And a.c, b.c, c.c all include common.h
# When the user PUSHes on 'counter' in a.c and NEXTs through all references
# And common.h becomes stale on the last NEXT
# Then all three CUs are reparsed
# And navigation wraps to common.h with shifted positions

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
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
