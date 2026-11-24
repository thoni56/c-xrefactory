# Scenario: a running server writes its command log
#
# Given the server writes the requests it gets to a command log
# When it has answered a request and is still running
# Then the command log already has its command line and the request

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer commands.log running.log .c-xref
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -E -- "c-xref |^-getproject" running.log | sed -E "s|^[^ ]*c-xref |c-xref |" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
