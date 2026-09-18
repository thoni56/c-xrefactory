# Browsing from inside an unexpanded macro body, with no batch -create
#
# Same as test_browsing_push_in_unexpanded_macro, but the server starts cold:
# no snapshot, nothing primed. Under "memory is truth" it should give the same
# answer, finding a CU that expands GET_AREA by itself.
#
# It does not. findMacroExpansionFile looks for a UsageMacroBaseFileUsage
# reference, and expandMacroCall only records one when options.mode == XrefMode
# (src/yylex.c), which is what -create runs. Removing that condition does make
# the reference appear, but the answer is still "No reference", so at least one
# more thing is missing on the server-only path.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref CXrefs output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
