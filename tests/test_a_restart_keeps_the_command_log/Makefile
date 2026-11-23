# Scenario: a restarted server adds to its command log
#
# Given the server writes the requests it gets to a command log
# When it answers a request, exits, and starts again to answer another
# Then the command log has the requests of both runs
# And each run starts with the command line the server was started with

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer commands.log .c-xref
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -E -- "c-xref |^-getproject" commands.log | sed -E "s|^[^ ]*c-xref |c-xref |" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
