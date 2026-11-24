# Scenario: a rename with conflicts leaves the browsing stack as it was
#
# Given a push on the call of twice() in main()
# When the parameter value is renamed to number, which conflicts with the static number
# And the client shows the conflicts, continues, and answers No to "really continue?"
# And a next follows
# Then the next goes to the call of twice(), in the push's references
# And a pop goes back to the call of twice(), where the push was made
#
# The rename shows its conflicts as "Conflicting References" in a dialog. After the
# dialog the stack is as before the rename, as when a refactoring was its own process.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@cp main.c.original main.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
