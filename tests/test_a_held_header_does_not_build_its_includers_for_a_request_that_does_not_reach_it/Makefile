# Scenario: a held header does not build its includers for a request that does not reach it
#
# Given a.c includes h.h, and z.c includes nothing
# And a push in a.c built it
# When a push in z.c holds h.h with new content
# Then a.c is not built, since the push does not reach h.h
# And a.c stays out of date, so a request that reaches h.h builds it (ADR-0035)

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log .c-xref
	@touch -t 200001010000 a.c z.c h.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "Siblings: .*\|'[^']*' is out of date.*" log | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
