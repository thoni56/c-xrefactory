include ../Makefile.server

# Scenario: the dump says which files have knowledge
#
# Given a.c includes x.h, and b.c includes nothing
# When the project is opened and a.c is parsed, by a push in it
# Then a dump of the knowledge says a.c has knowledge
# And x.h, a header, and b.c, never parsed, have none

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
