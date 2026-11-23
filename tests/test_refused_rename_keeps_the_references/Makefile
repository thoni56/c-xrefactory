# Scenario: a refused rename leaves the references as they are on disk
#
# Given total and sum, two globals declared in globals.h and defined in source.c,
#   and total also used in other.c
# When total is renamed to sum, which conflicts
# And the client answers No to "really continue?", so nothing is edited
# Then a push on total finds the five references of total
# And a push on sum finds the four references of sum
#
# The safety check builds the renamed source into the table before it undoes
# the rename (backlog #117).

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
