include ../Makefile.cli

# Scenario: the backlog drawn as an anatomy
#
# Given a backlog with an item in each of two sections
# And the second item waits for the first
# When the graph is generated
# Then each section is a cluster, each item a node named by its number and title
# And the edge goes from the first item up to the one that waits for it
# And the first is ready, the second blocked

$(TEST):
	@-../../utils/backlog_graph backlog.md > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
