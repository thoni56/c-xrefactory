# Scenario: a rename leaves the browsing stack as it was
#
# Given a push on the call of twice() in main()
# When the parameter value is renamed, without conflicts, and the client saves main.c
# And a pop follows
# Then the pop goes back to the call of twice(), where the push was made
#
# A rename pushes three sessions for its safety check (getReferences(),
# makeSafetyCheckAndUndo(), safetyCheck()). They used to die with the -refactory
# process, but in the server they stay on the stack unless popped.

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
