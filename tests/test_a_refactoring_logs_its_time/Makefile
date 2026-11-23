# Scenario: a refactoring logs its time, as every request does
#
# Given the server logs at -info
# When it answers a request for the project, a rename and a push
# Then the log has one line for each, naming the operation and its time

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log .c-xref
	@cp main.c.original main.c
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "Request .* answered in [0-9]* ms" log | sed -E "s/[0-9]+ ms/N ms/" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
