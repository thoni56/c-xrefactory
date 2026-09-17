# Completion inside a macro definition uses how the macro is used
#
# Inside #define FIELD(q) q-> the parameter q has no type of its own. The macro
# is used as FIELD(p) with a struct point *, so completion lists its fields.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
