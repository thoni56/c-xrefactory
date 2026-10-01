include ../Makefile.server

$(TEST):
	@# Test: a directory the config lists outside the project root is scanned,
	@# so its files belong to the project and -getproject accepts them
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

clean: clean-project-snapshot
clean-project-snapshot:
	@-rm -rf project/.c-xref
