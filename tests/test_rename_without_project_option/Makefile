# A refactoring without -p uses the discovered project
#
# The Emacs client sends -p with every request, but a client that relies on
# project discovery (the system tests, an LSP adapter) does not. This is the
# sibling case of test_rename_global_collision_through_includes without -p, and
# must give the same answer.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
