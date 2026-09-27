include ../Makefile.server

# Scenario: a file outside any project leaves no snapshot behind
#
# Given a file with no .c-xrefrc above it
# When the client asks for its project and the server exits
# Then the answer is that there is no project
# And no snapshot is written, neither CXrefs nor .c-xref
#
# The boilerplate generates a .c-xrefrc here, so the test removes it.

$(TEST):
	@rm -f .c-xrefrc
	@rm -rf .c-xref CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@ls -d CXrefs .c-xref >> output.tmp 2>/dev/null || true
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
