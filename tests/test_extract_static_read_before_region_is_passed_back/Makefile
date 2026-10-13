include ../Makefile.server

# Scenario: a static local that the region writes and the function reads before it
#
# Given compute() remembers its last argument in a static local
# And reads the remembered value before the region
# When the line that stores the new value is extracted
# Then the static is passed in and its new value returned to it
#
# Within one call the write looks dead, since nothing after the region reads it,
# but the next call does. Treated as a local, the extracted function would get a
# static of its own, and the one in compute() would never change.

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace'
trace: $(TEST)

GDB_COMMANDS =
