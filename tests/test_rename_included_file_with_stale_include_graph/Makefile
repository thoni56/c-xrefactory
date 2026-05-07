# Positive regression test: Pass 3 (parseUnparsedSiblingCUs) self-heals stale
# include-graph entries for ghost includers — files that used to include a
# header and no longer do.
#
# Scenario:
#   1. a.c and b.c both #include "common.h".
#   2. Server starts. -olcxpush on b.c parses b.c; Pass 3 visits a.c as a
#      sibling sharing common.h, parses it. Memory has include refs for both.
#   3. a.c is replaced on disk with a version that does NOT include common.h
#      (natural mtime change, no preservation).
#   4. -rfct-rename-included-file is requested on b.c (cursor on its #include).
#
# Expected behaviour (verified): Pass 3 fires again at the start of the
# rename-included-file request, sees a.c's mtime has moved past
# lastParsedMtime, reparses a.c, strips the now-defunct include ref. The
# refactoring then walks LINK_NAME_INCLUDE_REFS[common.h] and finds only
# b.c. Output emits a <replacement> for b.c only.
#
# This test guards against future regressions where Pass 3 stops covering
# this self-heal. It does NOT cover the "missing includer" case (a file
# that newly added a #include but isn't in the refs list yet) — that's a
# fresh-disk-scan problem, not addressable by walking existing refs.

include ../Makefile.server

ORIGINAL = a.original

$(TEST):
	@cp $(ORIGINAL) a.c
	@-rm -rf .c-xref
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
