# Scenario: a rename leaves nothing to re-push
#
# Given a push on the call of twice() in main()
# When the parameter value is renamed, without conflicts, and the client saves main.c
# And a re-push follows
# Then there is nothing to re-push
#
# A pop only moves the top of the browsing stack, and a re-push moves it up again.
# So the sessions a refactoring pops are still there, unless they are deleted.

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
