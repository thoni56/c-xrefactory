PREPARE = prepare

include ../Makefile.server

# Verify that -prune directives are honored by the recursive project scan
# in ServerMode. The tree has three .c files at different depths:
#   source.c                       (top level)            -> must be found
#   included/included_source.c     (non-pruned subdir)    -> must be found
#   pruned/pruned_source.c         ('pruned' is -prune'd) -> must NOT be found
# We count .c-file entries the snapshot recorded under this test directory.
# Expected: 2. This distinguishes all three states:
#   - no recursion           -> 1 (only source.c)        FAIL
#   - recursion, no pruning  -> 3 (pruned included)       FAIL
#   - recursion + pruning    -> 2 (pruned excluded)       PASS

$(TEST):
	@$(SERVER_DRIVER) commands.input > output.tmp
	@grep "$(CURDIR)" .c-xref/db | grep "\\.c" | wc -l | tr -d ' ' > output
	$(VERIFY)

prepare:
	@rm -rf .c-xref

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
