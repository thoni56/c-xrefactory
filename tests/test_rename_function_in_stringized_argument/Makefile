# expect() in Cgreen stringizes the name of the function it mocks, so the name
# reaches the parser only as a string. The identifier in the source is still a
# reference to the function, and a rename must edit it.
#
# The two macros have the shape of Cgreen's, so that the test does not need it
# installed.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
