include ../Makefile.server

# Scenario: a restart leaves a CU's includes as its parse recorded them
#
# Given a.c includes x.h only inside an #ifdef that is false
# And b.c includes x.h unconditionally on line 3
# When both are parsed in a first session
# And a second session opens the project, where both are current
# Then the snapshot is the same after both sessions: no include of x.h from a.c, and one
#   from b.c, on line 3
#
# The second session's scan adds an include at line 1, column 0 of both CUs.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA1) > session1.output 2>&1
	@echo "--- after session 1" > output
	@../../utils/cxref_reader .c-xref/db >> output
	@-$(SERVER_DRIVER) commands.input $(EXTRA2) > session2.output 2>&1
	@echo "--- after session 2" >> output
	@../../utils/cxref_reader .c-xref/db >> output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =
