# typeof() gives a declaration the type of an expression or a type name. c-xref
# predefines typeof() and __typeof__() as int (src/startup.c), so the name inside is
# not a reference, and a field of the declared variable is not found. A push from
# each shows the reference: the field through typeof(*p), the name inside
# __typeof__(origin), and the field through that.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
