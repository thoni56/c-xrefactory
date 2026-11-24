# offsetof() names a field, but c-xref predefines offsetof() and __builtin_offsetof()
# for the parser (src/startup.c). If those throw their arguments away, the field is not
# a reference. A push from the field in each of them shows the reference.
#
# No system header is included, so the test sees c-xref's own definitions.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
