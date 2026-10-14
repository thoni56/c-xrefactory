include ../Makefile.server

# Scenario: a push in a source with CR LF line endings
#
# Given a source whose lines end in CR LF
# When the client pushes on the call to answer(), with the cursor offset counted
#   the way the editor counts it, one character per line break
# Then the server lands on answer() and goes to its definition
#
# Counting the CRs, the offset would land seven characters earlier, on "return".

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
