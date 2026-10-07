include ../Makefile.server

# Scenario: knowledge from content held in the editor does not survive a restart
#
# Given a.c is held in the editor with unsaved changes, and parsed from them
# Then a.c has knowledge
# When the server exits, which writes the snapshot, and a second session opens the project
# Then a.c has no knowledge, since the content it was built from was never saved

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@touch -t 200001010000 a.c
	@-$(SERVER_DRIVER) commands_session1.input $(EXTRA1) > output.tmp 2>&1
	@-$(SERVER_DRIVER) commands_session2.input $(EXTRA2) >> output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =
