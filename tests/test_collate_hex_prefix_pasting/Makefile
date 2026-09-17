# 0x in a macro body is one token
#
# In C, 0x is a single preprocessing number, so in 0x##x only the second x is the
# parameter. The expected intermediate entries (0x##FF, 0xFF##U, 0x##BEEF) keep
# the positions the current, wrong, output has; check them when fixing. The
# final numbers 0xFFU and 0xBEEF must not be symbols.

include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
