# Scenario: two undone refactorings leave the references as they are on disk
#
# Given popOnly() and popSession(), declared in stack.h and defined in stack.c,
#   and called from user.c
# When popOnly() is renamed to popSession, which conflicts, and the client continues
# And a parameter is added to popSession() while the client holds the renamed buffers
# And the client undoes both, without saving, so its buffers are as on disk and unmodified
# Then a push on popSession in stack.c finds only the three references of popSession
#
# Seen in daily use: a later refactoring on popSession() edited the calls of the
# old name (backlog #117).

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
