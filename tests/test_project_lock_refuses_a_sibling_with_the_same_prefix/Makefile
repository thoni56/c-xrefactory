include ../Makefile.server

# Given a server locked to project_a
# When a file in project_ab, whose path starts with project_a's, asks for its project
# Then the server says it is locked to another project

$(TEST):
	@rm -rf project_a/.c-xref
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

clean: clean-project-snapshot
clean-project-snapshot:
	@-rm -rf project_a/.c-xref
