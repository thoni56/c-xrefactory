include ../Makefile.server

# Scenario: -xrefactory-II on the server's command line is an error, reported once
#
# Given the server is started with -server -xrefactory-II
# When the client asks for the project and then pushes
# Then the -getproject answer says -xrefactory-II is no longer supported
# And nothing is written before the first request
# And the -push answer does not say it again

$(TEST): .c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
