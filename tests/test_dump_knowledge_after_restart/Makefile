include ../Makefile.server

# Scenario: knowledge survives a restart
#
# Given a.c includes x.h, and b.c includes nothing
# And a.c was parsed in a first session, which wrote the snapshot at exit
# When a second session opens the project, which loads the snapshot and parses nothing
# Then a dump of the knowledge says a.c has knowledge
# And x.h and b.c have none

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands_session1.input $(EXTRA1) > session1.output 2>&1
	@-$(SERVER_DRIVER) commands_session2.input $(EXTRA2) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =
