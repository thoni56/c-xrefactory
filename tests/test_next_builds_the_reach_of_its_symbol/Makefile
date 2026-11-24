include ../Makefile.server

# Scenario: Next builds the reach of its symbol, wherever the cursor is
#
# Given a.c and b.c include sym.h, which declares shared, and c.c includes nothing
# And a push on shared in a.c
# When b.c gets a new first line on disk, so its reference to shared moves down a line
# And Next is done four times with the cursor in c.c
# Then the Nexts go to every reference of shared, the one in b.c on its new line
#
# The goal of a Next is the reach of the session's symbol, not of the buffer it is sent
# from. c.c shares no header with b.c, so the buffer's siblings leave b.c out.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	@cp b.c.v1 b.c
	@touch -t 200001010000 sym.h a.c b.c c.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
