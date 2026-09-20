# A macro body browse must not cost the file its references
#
# Nothing includes only.c and nothing expands DOUBLE, so there is no compilation
# unit that could give `value` in the macro body a meaning. Answering "no
# reference" is correct. What is not correct is losing the references only.c
# already has: the second push, on the declaration itself, must still work.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@../../utils/cxref_reader .c-xref/db >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
