include ../Makefile.cli

# Scenario: the check that keeps the backlog's names honest
#
# Given a backlog whose items name only items that exist, and one naming items that do not
# When --check runs on each
# Then the first passes silently
# And the second fails, naming each unknown item

$(TEST):
	@-../../utils/backlog_graph --check ok.md > output.tmp 2>&1; echo "known names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unknown.md >> output.tmp 2>&1; echo "unknown names: exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
