include ../Makefile.cli

# Scenario: the backlog's graph as a page of its own
#
# Given a backlog whose items wait for, and go with, other items
# When the graph is written as HTML
# Then the page holds the graph, escaped, for Mermaid to draw with ELK at full size
# And / opens a search that pans to each matching item in turn
# And + and - zoom, the arrow keys pan, and 0 fits the whole graph

$(TEST):
	@-../../utils/backlog_graph --html backlog.md > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
