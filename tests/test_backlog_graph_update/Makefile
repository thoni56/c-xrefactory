include ../Makefile.cli

# Scenario: the graph is kept inside the backlog
#
# Given a backlog without a graph
# When --update runs, and then runs again
# Then the graph is added once at the end, between its markers, as a mermaid block
# And the second run replaces it instead of adding another

$(TEST):
	@cp backlog.md work.md
	@-../../utils/backlog_graph --update work.md > output.tmp 2>&1
	@-../../utils/backlog_graph --update work.md >> output.tmp 2>&1
	@cat work.md >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
