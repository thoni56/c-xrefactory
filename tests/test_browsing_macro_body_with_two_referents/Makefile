# An identifier in a macro body means whatever the expanding CUs make it mean
#
# `next` in node.h is a field of struct list_node in list.c and a field of
# struct tree_node in tree.c. Both are true, so the answer is the browser menu
# with both, and never one of them picked silently (ADR-0027).
#
# The server starts cold: no snapshot, nothing primed. The expanding CUs are
# found by walking the includers of the file defining the macro.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
