include ../Makefile.server

# Scenario: Session positions update when POPping to a session after buffer modification
#
# Given source.c has been indexed with -create
# And the user PUSHes on 'function1' (creating session 1)
# And the user PUSHes again on 'function1' (creating session 2)
# When a modified buffer is sent with NEXT (blank line inserted before the call)
# Then session 2 navigates to the shifted call site (line 7)
# When the user POPs (returning to session 1)
# Then POP returns the shifted caller position (line 7, not stale line 6)
# When the user NEXTs on session 1
# Then session 1 also navigates to the shifted call site (line 7)
#
# WHY SUSPENDED: Sessions hold copies of references, not indexes into the
# live in-memory table. When a reparse refreshes one session, other sessions
# on the stack retain stale copies. The per-file needsBrowsingStackRefresh
# flag is consumed by the first session that checks it. Fixing this properly
# requires either "index-based sessions" (sessions read from the live table)
# or "memory-is-truth" (no disk-db-only refs that could be lost on full
# session rebuild). Both are on the roadmap.

$(TEST):
	$(CXREF) -create > /dev/null 2>&1
	touch source.preload # ensure the preloaded buffer has a more recent timestamp
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=log"
trace: $(TEST)

GDB_COMMANDS =
