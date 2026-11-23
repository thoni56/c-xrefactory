include ../Makefile.cli

# Scenario: the check that keeps the backlog's names honest
#
# Given a backlog whose items name only items that exist, also in its Foundation, one
#   naming items that do not, one where two items have the same number, one where an
#   item's heading has no name, and one where an item comes after two others, and one
#   where an item waits for and comes after an item split into parts, and one where items
#   wait for each other in a loop
# When --check runs on each
# Then the first passes silently
# And the second fails, naming each unknown item
# And the third fails, naming the items that share the number
# And the fourth fails, naming the item that is not read as one
# And the fifth fails, since an item comes after one other at most
# And the sixth fails, since an item with parts is not drawn, so the order names a part
# And the seventh fails, naming the items in the loop, since a loop has no order

$(TEST):
	@-../../utils/backlog_graph --check ok.md > output.tmp 2>&1; echo "known names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unknown.md >> output.tmp 2>&1; echo "unknown names: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check duplicate.md >> output.tmp 2>&1; echo "same number: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check unnamed.md >> output.tmp 2>&1; echo "no name: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check two_afters.md >> output.tmp 2>&1; echo "two afters: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check parent.md >> output.tmp 2>&1; echo "an item with parts: exit=$$?" >> output.tmp
	@-../../utils/backlog_graph --check cycle.md >> output.tmp 2>&1; echo "a loop: exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
