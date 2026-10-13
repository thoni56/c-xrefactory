include ../Makefile.server

# Scenario: a search result number of zero is answered, not a crash
#
# Given a search has listed the matches for int*
# When the client asks to go to, or select, result number 0
# Then the server answers with an error naming the number
# And goes to result number 1 after that
#
# The client sends 0 when the result buffer is empty (backlog
# search-result-line-zero), and the server used to assert on it.

$(TEST):
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
