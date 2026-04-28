include ../Makefile.server

# Scenario: snapshot has a corrupted usage ref (simulating snapshot drift).
# The client preloads source.c (simulating a dirty emacs buffer) so the
# server's Pass 1 reparse calls removeReferenceableItemsForFile, giving a
# clean in-memory table. Menu creation must read from that in-memory table,
# not re-scan the stale disk snapshot — otherwise the corrupted ref leaks
# into the rename occurrences list and the validation pass aborts.

$(TEST):
	@-rm -rf .c-xref
	@mkdir -p .c-xref
	# Populate a real snapshot with F's references
	@$(SERVER_DRIVER) setup.input > /dev/null
	# Corrupt the call-site ref: line 6 col 11 -> line 5 col 4
	@sed -i.bak '/2\/F/s/5u6l11cr/5u5l4cr/' .c-xref/db
	# Ensure preload mtime > source.c mtime so Pass 1 considers it stale
	@touch source.preload
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
