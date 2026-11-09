include ../Makefile.server

# Scenario: a header held in the editor at an exit is changed after a restart
#
# Given header.h declares shared, and a.c includes it
# And a push held header.h with changed content, which moves shared to line 2
# And the server exits while header.h is still held
# When a second session, which holds nothing, pushes by name on shared
# Then shared is found at its declaration in header.h on disk, on line 1

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer .c-xref
	@touch -t 200001010000 header.h a.c
	@-$(SERVER_DRIVER) commands_session1.input $(EXTRA1) > output.tmp 2>&1
	@-$(SERVER_DRIVER) commands_session2.input $(EXTRA2) >> output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =
