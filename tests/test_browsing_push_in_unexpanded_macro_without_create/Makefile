# Browsing from inside an unexpanded macro body, with no batch -create
#
# Same as test_browsing_push_in_unexpanded_macro, but the server starts cold:
# no snapshot, nothing primed. Under "memory is truth" it should give the same
# answer, finding a CU that expands GET_AREA by itself.
#
# It finds one by walking the include graph from macro.h to source.c, which
# -getproject has already parsed. The -push parses source.c again, with the
# cursor in the macro body, to resolve calculate_area.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
