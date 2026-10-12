include ../Makefile.cli

# Scenario: the check that keeps the backlog's names honest
#
# Given a backlog whose items name only items that exist, one naming items that do not,
#   and one where two items have the same number
# When --check runs on each
# Then the first passes silently
# And the second fails, naming each unknown item
# And the third fails, naming the items that share the number

$(TEST):
	@-../../utils/backlog_graph --check ok.md > output.tmp 2>&1; echo "known names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unknown.md >> output.tmp 2>&1; echo "unknown names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check duplicate.md >> output.tmp 2>&1; echo "same number: exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
