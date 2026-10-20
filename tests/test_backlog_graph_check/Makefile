include ../Makefile.cli

# Scenario: the check that keeps the backlog's names honest
#
# Given a backlog whose items name only items that exist, one naming items that do not,
#   one where two items have the same number, one where a title is on two lines, and one
#   where an item comes after two others
# When --check runs on each
# Then the first passes silently
# And the second fails, naming each unknown item
# And the third fails, naming the items that share the number
# And the fourth fails, naming the item that is not read as one
# And the fifth fails, since an item comes after one other at most

$(TEST):
	@-../../utils/backlog_graph --check ok.md > output.tmp 2>&1; echo "known names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unknown.md >> output.tmp 2>&1; echo "unknown names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check duplicate.md >> output.tmp 2>&1; echo "same number: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check wrapped.md >> output.tmp 2>&1; echo "title on two lines: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check two_afters.md >> output.tmp 2>&1; echo "two afters: exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
