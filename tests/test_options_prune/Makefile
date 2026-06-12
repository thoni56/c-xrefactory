PREPARE = prepare

include ../Makefile.server

# Verify that -prune directives are honored by the lightweight scan
# in ServerMode: the snapshot must NOT contain entries for files
# under 'pruned/'. The directory is registered as -prune in the
# generated .c-xrefrc, and we count .c-file entries the snapshot
# recorded under this test directory (expected: 1 == source.c only).

$(TEST):
	@$(SERVER_DRIVER) commands.input > output.tmp
	@grep "$(CURDIR)" .c-xref/db | grep "\\.c" | wc -l | tr -d ' ' > output
	$(VERIFY)

prepare:
	@rm -rf .c-xref

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
