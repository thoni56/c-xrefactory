include ../Makefile.server

# Scenario: a push in a header changed on disk keeps the references its includers put there
#
# Given a.c defines the macro T and then includes a.h, which uses T
# And a.c has been parsed
# When a.h is rewritten on disk, with its line one down
# And f is pushed on in a.h
# Then T, f and a.h itself are still referenced in a.h
#
# a.h on its own does not know T is a macro, so parsing it alone cannot put T back. The
# test lists which symbols have a reference in a.h, not where, since the old lines stay
# until the includers are parsed again.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp a.h.v1 a.h
	@touch -t 200001010000 a.h a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@../../utils/cxref_reader .c-xref/db | awk '/^[^ ]/ {symbol = $$0} /@a\.h:/ {print symbol}' | LC_ALL=C sort -u > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
