include ../Makefile.server

# Scenario: a push by name analyses a few files without asking
#
# Given main.c includes nothing, and user.c defines value
# And nothing has been analysed yet
# When value is pushed by name from main.c
# Then it goes to the definition in user.c, without asking first
#
# Analysing two files is well within the cap, so there is nothing to wait for and
# nothing to ask about (ADR-0032, time budget).

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
