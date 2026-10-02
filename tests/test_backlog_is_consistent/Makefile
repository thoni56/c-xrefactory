include ../Makefile.cli

# Scenario: the backlog's names and graph agree with its text
#
# Given doc/backlog.md, where items name what they wait for and go with
# When utils/backlog_graph checks it
# Then every name it uses exists, and the graph at its end is up to date
#
# If this fails, the message says which name is unknown, or that the graph needs
# utils/backlog_graph --update doc/backlog.md

$(TEST):
	@-../../utils/backlog_graph --check ../../doc/backlog.md > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
