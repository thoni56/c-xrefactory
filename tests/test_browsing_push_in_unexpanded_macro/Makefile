# Browsing from inside an unexpanded macro body
#
# The cursor is on calculate_area inside the body of GET_AREA in macro.h, which
# is not a symbol until the macro is expanded. The server starts cold, with no
# snapshot, and finds a CU that expands the macro by walking the include graph
# from macro.h to source.c, which -getproject has already parsed. The -push
# parses source.c again, with the cursor in the macro body, to resolve
# calculate_area.
#
# The identifier has one referent, so the answer is a goto. Several would give
# the browser menu instead - olcxShowSelectionMenu() decides on the number of
# distinct referents, and a macro body is no exception to that.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
