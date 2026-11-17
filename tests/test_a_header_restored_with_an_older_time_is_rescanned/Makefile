include ../Makefile.server

# Scenario: a header restored with an older time is scanned again
#
# Given a.c includes h.h, which includes x.h
# And b.c includes y.h, which declares g, and defines g
# And a.c uses g
# When h.h is replaced by an older version, with its older time, which includes y.h
# And g is pushed on in b.c
# Then the use of g in a.c is among its references
#
# The restored time is not after the scan, so "newer" does not scan h.h again,
# and a.c is not found to include y.h. A difference does (ADR-0036).

$(TEST): .c-xrefrc
	@cp h.h.current h.h
	@touch -t 200001010000 h.h.restored
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
