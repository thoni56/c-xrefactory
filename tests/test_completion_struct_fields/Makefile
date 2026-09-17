# Completion lists the fields of a struct
#
# Both after p-> on a pointer variable and after . on a complex expression whose
# type comes from a cast, so completion has to work out the type of the whole
# expression rather than look up a variable.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
