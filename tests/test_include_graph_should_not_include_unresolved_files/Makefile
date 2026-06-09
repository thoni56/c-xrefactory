include ../Makefile.server

# Asserts the lightweight project scan does NOT register file-table
# entries for #include paths that can't be resolved on disk. Currently
# fails because projectstructure.c:resolveIncludePath returns a phantom
# (includer-dir + include-path) when nothing resolves, and the caller
# adds that phantom to the file table unconditionally — those entries
# end up in the snapshot.
#
# The greppable proof: the snapshot should not mention nonexistent.h.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@(grep "nonexistent.h" .c-xref/db; true) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
