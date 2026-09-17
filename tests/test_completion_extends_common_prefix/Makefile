# Completion extends to the longest common prefix
#
# part matches partial_one and partial_two, so completion extends it to
# partial_ and reports that there are multiple completions. The candidates are
# local declarations, so the answer does not depend on the standard library.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
