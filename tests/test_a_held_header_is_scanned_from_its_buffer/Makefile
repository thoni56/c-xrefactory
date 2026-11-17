include ../Makefile.server

# Scenario: a held header is scanned from its buffer
#
# Given a.c includes h.h, which on disk includes x.h
# And b.c includes y.h, which declares g, and defines g
# And a.c uses g
# When h.h is held with content that includes y.h, the disk unchanged
# And g is pushed on in b.c
# Then the use of g in a.c is among its references
#
# The parse reads the held content, so the scan has to read it too, or a.c is
# not found to include y.h.

$(TEST): .c-xrefrc
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
