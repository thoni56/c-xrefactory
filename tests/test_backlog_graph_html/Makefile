include ../Makefile.cli

# Scenario: the backlog's graph as a page of its own
#
# Given a backlog whose items wait for, and go with, other items
# When the graph is written as HTML
# Then the page holds the graph, escaped, for Mermaid to draw at full size

$(TEST):
	@-../../utils/backlog_graph --html backlog.md > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
