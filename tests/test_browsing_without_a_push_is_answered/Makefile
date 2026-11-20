include ../Makefile.server

# Scenario: browsing before any push is answered
#
# Given a server that has not pushed, e.g. one just restarted
# When the client asks to pop, go to the next or previous reference, or re-push
# Then the server answers each of them, and does not assert on browsingStack.top
#
# The client keeps its keys for the browser in source buffers, so they can reach
# a server that has no browsing stack yet.

$(TEST):
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
