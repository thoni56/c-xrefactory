include ../Makefile.server

# Scenario: a file parsed for an operation counts as parsed
#
# Given a.c and b.c both include f.h, so -getproject on a.c parses b.c
# And a push in a.c parses a.c itself
# When browsing f by name, which checks every compilation unit first
# Then nothing needs reparsing, so there is no question and it goes to f

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
