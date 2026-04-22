include ../Makefile.server

# Scenario: a snapshot contains a reference whose stored position no longer
# aligns with the function name in the file (simulating stale db). Rename
# should detect the mismatch and abort — not proceed to mutate the file.

$(TEST):
	@-rm -rf .c-xref
	@mkdir -p .c-xref
	# Populate a real snapshot with F's references
	@$(SERVER_DRIVER) setup.input > /dev/null
	# Corrupt the call-site ref: line 6 col 11 -> line 5 col 4
	# (a position that in source.c resolves to 'main', not 'F')
	@sed -i '/2\/F/s/5u6l11cr/5u5l4cr/' .c-xref/db
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
