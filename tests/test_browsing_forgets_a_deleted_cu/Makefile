# A compilation unit deleted outside the editor leaves no references behind
#
# Given a.c and b.c both include f.h, so -getproject parses both, and b.c calls f
# When b.c is deleted on disk, with no editor involved
# Then the next push says nothing about it, and browsing f no longer lists the call
# And a push by name, which checks every compilation unit, finds nothing left to
#   reparse, so b.c is forgotten, not retried

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp b.c.original b.c
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
