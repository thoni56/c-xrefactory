include ../Makefile.cli

# Scenario: the backlog's names exist
#
# Given doc/backlog.md, where items name what they wait for and go with
# When utils/backlog_graph checks it
# Then every name it uses exists
#
# If this fails, the message says which name is unknown

$(TEST):
	@-../../utils/backlog_graph --check ../../doc/backlog.md > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)
