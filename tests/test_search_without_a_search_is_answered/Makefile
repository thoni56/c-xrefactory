include ../Makefile.server

# Scenario: going to a search result before any search is answered
#
# Given a server that has not searched, e.g. one just restarted
# When the client asks to go to, or select, result number 1
# Then the server answers with an error saying there are no results
#
# The client keeps the result list of a server that has stopped, and
# the new server used to assert on searchingStack.top.

$(TEST):
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
