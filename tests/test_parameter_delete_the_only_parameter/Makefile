include ../Makefile.server

# Scenario: deleting a function's only parameter leaves (void)
#
# Given function takes one parameter, and main calls it
# When the parameter is deleted
# Then the definition says (void), since before C23 () declares a function
#   without a prototype, and the call has no argument

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
