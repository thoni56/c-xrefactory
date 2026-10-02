include ../Makefile.cli

# Scenario: the check that keeps the backlog's graph honest
#
# Given a backlog whose graph is up to date, one without a graph,
#   and one naming items that do not exist
# When --check runs on each
# Then the first passes silently
# And the others fail, saying the graph is out of date and naming each unknown item

$(TEST):
	@cp ok.md work.md
	@../../utils/backlog_graph --update work.md
	@-../../utils/backlog_graph --check work.md > output.tmp 2>&1; echo "up to date: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check ok.md >> output.tmp 2>&1; echo "no graph: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unknown.md >> output.tmp 2>&1; echo "unknown slug: exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
