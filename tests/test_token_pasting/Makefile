include ../Makefile.server

# This test token pasting which mostly works. It does not include token pasting into
# numbers, which doesn't. The suspended test_token_pasting_numbers,
# test_token_pasting_float and test_collate_hex_prefix_pasting cover that.

$(TEST):
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
