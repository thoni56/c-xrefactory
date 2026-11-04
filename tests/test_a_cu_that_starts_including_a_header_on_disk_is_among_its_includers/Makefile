include ../Makefile.server

# Scenario: a CU that starts including a header on disk is found from the header
#
# Given b.c includes h.h and calls f, which h.h declares
# And a.c does not include h.h
# And f is pushed on in h.h
# When a.c is rewritten on disk to include h.h and call f
# And f is pushed on in h.h again
# Then the call in a.c is among its references

$(TEST): .c-xrefrc
	@cp a.c.v1 a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
