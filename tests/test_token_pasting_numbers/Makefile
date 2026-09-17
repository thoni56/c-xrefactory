# A number formed by ## is not a symbol
#
# Pasting that forms an identifier records the result as a symbol, and the
# intermediate a##b entries too. Pasting that forms a number should record
# neither 10 nor 2e10 as a symbol. The intermediate entries (1##0, 2##e, 2e##10)
# are kept for now; whether numbers need them is decided when this is fixed.

include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
