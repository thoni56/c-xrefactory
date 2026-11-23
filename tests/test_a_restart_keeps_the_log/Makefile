# Scenario: a restarted server adds to its log
#
# Given the server logs at -info to a file
# When it answers a request, exits, and starts again to answer another
# Then the log has the lines of both runs
#
# A restart is often what follows a problem, so it must not remove the log of the
# run that had it.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log .c-xref
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "Request .* answered in [0-9]* ms" log | sed -E "s/[0-9]+ ms/N ms/" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
