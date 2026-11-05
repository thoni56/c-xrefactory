# Scenario: a held CU that starts including a header is rebuilt when the header changes
#
# Given a.c on disk does not include h.h
# And a.c is held with content that includes h.h and expands CALL, which h.h defines as f
# And b.c includes h.h and calls f
# When h.h is rewritten on disk with a line inserted above CALL
# And f is pushed on in h.h, with a.c still held
# Then the use of f in the body of CALL, which only a.c puts there, is among its references
#
# The edge from a.c to h.h is in the held content only, not on disk. The sleeps
# keep each build more than the coarse clock's 10 ms after the change before it.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp a.c.on_disk a.c
	@cp h.h.before h.h
	@touch -t 200001010000 a.c b.c h.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
