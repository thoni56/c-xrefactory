include ../Makefile.server

$(TEST):
	@# Test: server locks to first project and rejects files from other projects
	@rm -rf project_a/.c-xref
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

clean: clean-project-snapshot
clean-project-snapshot:
	@-rm -rf project_a/.c-xref
