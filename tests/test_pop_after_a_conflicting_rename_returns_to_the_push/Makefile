# Scenario: a rename with conflicts leaves the browsing stack as it was
#
# Given a push on the call of twice() in main()
# When the parameter value is renamed to number, which conflicts with the static number
# And the client shows the conflicts, continues, and answers No to "really continue?"
# And a pop follows
# Then the push on twice() is on top again
# And a second pop goes back to the call of twice(), where the push was made
#
# The rename shows its conflicts as "Conflicting References". Under that the rename's
# references (getReferences()) and the request's session used to stay on the stack.

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
