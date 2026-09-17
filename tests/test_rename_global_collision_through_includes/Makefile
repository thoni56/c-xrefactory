# Rename's safety check sees collisions reachable through includes
#
# Rename checks whether any reference would bind to a different symbol after the
# rename. It does not parse the whole project, so it only sees what include-based
# discovery parses.
#
# sibling: main.c includes both counter.h and total.h, so renaming counter to
# total merges it with the global total. The check must warn.
#
# unrelated: elsewhere.c defines a global elsewhere but shares no header with
# anything, so renaming counter to elsewhere is not detected. That clash only
# shows up when linking, the same accepted trade-off as ADR-0013. The check does
# not warn, and this test pins that down so a change is noticed.
#
# Each case runs in its own server, so both start cold.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) sibling.input $(EXTRA) > output.tmp
	@rm -rf .c-xref
	@$(SERVER_DRIVER) unrelated.input $(EXTRA) >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
