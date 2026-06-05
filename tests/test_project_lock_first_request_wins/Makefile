include ../Makefile.server

# Bug exposure: when -getproject locks the project from a file in dir_a/,
# a subsequent request mentioning a file in dir_b/ (which has its own
# .c-xrefrc) should NOT re-detect dir_b as the project root. The project
# lock established by the first -getproject must be honored by all
# subsequent requests.
#
# Without the fix: server.c:584 fires initializeProjectContext for the
# -search request with dir_b's file, re-runs auto-detect, lands on
# dir_b/.c-xrefrc, and the snapshot ends up in dir_b/.c-xref/db.
# The cxref.c:1889 path (called from the -getproject handler) sets
# cxFileLocation transiently on the per-request options but never
# propagates to savedOptions — so the lock from -getproject doesn't
# survive the next request's deepCopyOptionsFromTo reset.
#
# After the fix: the project stays locked to dir_a, snapshot lands in
# dir_a/.c-xref/db, and dir_b/.c-xref/db is never created.

$(TEST):
	@rm -rf dir_a/.c-xref dir_b/.c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@ls dir*/.c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
