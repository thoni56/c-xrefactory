include ../Makefile.server

# Scenario: two locals declared in the extracted region and used after it
#
# Given a region that declares spaces and tabs in one declaration
# And code after the region that uses both
# When the region is extracted as a function
# Then they come back through out parameters
# And the call site still declares them, since the code after the call uses them

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
