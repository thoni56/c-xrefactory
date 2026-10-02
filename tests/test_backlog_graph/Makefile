include ../Makefile.cli

# Scenario: the backlog drawn as an anatomy
#
# Given a backlog whose items wait for, and go with, other items and the foundation
# When the graph is generated
# Then it holds only what takes part in an edge, labelled with section, number and title
# And each edge goes from what is waited for up to what waits
# And an item is ready, blocked or done, and a label's quotes and brackets are escaped

$(TEST):
	@-../../utils/backlog_graph backlog.md > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
