include ../Makefile.server

# Scenario: a static local that the region reads and writes
#
# Given compute() keeps a running count in a static local
# When the lines that update and read the count are extracted
# Then the static is passed by address, so the update reaches it,
# and the extracted function copies it into a plain local
#
# Passed by value, the extracted function would update its parameter
# and the static would never change again.

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace'
trace: $(TEST)

GDB_COMMANDS =
