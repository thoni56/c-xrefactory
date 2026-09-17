# Completion writes the rest of a list loop
#
# After for(l=first; completion offers the condition l!=NULL; and after the
# condition it offers the step l=l->next) {, using the struct's next field.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
