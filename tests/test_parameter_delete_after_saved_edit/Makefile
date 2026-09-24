# A parameter deletion after a saved hand edit sees the edit
#
# Delete f's parameter, then remove a line by hand, both saved to disk the way
# the editor does it, so nothing is preloaded. Deleting g's parameter must then
# find the call to g where the edited file has it, not where it used to be.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp source.c.original source.c
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
