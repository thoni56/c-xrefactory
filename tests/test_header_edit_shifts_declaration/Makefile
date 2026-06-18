# Regression test: an editor edit that MOVES a declaration in a header leaves no
# stale ("ghost") reference at the old position. Pins the previous cleaning concern.
#
# Scenario:
#   1. getproject + push parse source.c against header_v1.h  -> foo declared at
#      header.h line 3 (a reference now exists at that header position).
#   2. The editor "saves" header.h with two comment lines inserted, shifting the
#      declaration to line 5 — delivered the way a real editor does, via
#      -preload header.h header_v2.h (NOT a behind-the-back disk write; see the
#      "Editor Buffers Are the Content Source" principle).
#   3. A rename of foo must edit the declaration at its NEW position (line 5) and
#      nothing at the stale line-3 position.
#
# removeReferenceableItemsForFile(CU) cleans only the CU's own positions, but a
# header content change routes through the header's own reparse + strip, so the
# old header position is cleaned. A regression that left foo@3 would make the
# rename precheck hit the comment text now at line 3 -> abort or mis-edit, and
# this pinned output would change.

PREPARE = prepare
include ../Makefile.server

ORIGINAL = header_v1.h
HEADER = header.h

$(TEST):
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

prepare:
	@cp $(ORIGINAL) $(HEADER)
	@-rm -rf .c-xref

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
