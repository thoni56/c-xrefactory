include ../Makefile.server

# Scenario: source.c includes header.h. Cmd 2 preloads source.c (the
# user is editing it) and pushes references for `target`. Cmd 3 closes
# the source.c editor buffer (no longer preloaded — it becomes a ghost)
# and preloads header.h with a newer timestamp instead. Browsing through
# `target` should still find the use in source.c.
#
# Without proper handling, ghost cleanup strips source.c's refs
# (including the source.c -> header.h include edge). Pass 2 then can't
# find source.c as an includer of stale header.h, fails to reparse
# source.c, and target's use ref is lost.

$(TEST):
	@-rm -rf .c-xref
	@mkdir -p .c-xref
	@$(SERVER_DRIVER) setup.input > /dev/null
	@cp source.c source.preload
	@sleep 1
	@cp header.h header.preload
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
