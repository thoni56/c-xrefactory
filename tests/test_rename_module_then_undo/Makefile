include ../Makefile.server

# Scenario: Rename Module then simulate client undo.
# After the undo and a subsequent request, the snapshot should not
# contain file entries for the renamed (target) files.

$(TEST):
	@-rm -rf .c-xref
	@-rm -f target.c target.h
	@cp main.c.original main.c
	@cp source.c.original source.c
	@cp source.h.original source.h
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@-grep target .c-xref/db/XFiles 2>/dev/null >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
