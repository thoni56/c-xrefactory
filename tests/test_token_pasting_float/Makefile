# Pasting with '.' keeps the '.'
#
# x##.##y with 3 and 14 must form 3.14, not 314. The intermediate entries
# follow the pattern of identifier pasting; their exact spelling and positions
# are a best guess, since the current code never produces them.

include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
