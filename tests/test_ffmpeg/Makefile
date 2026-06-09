include ../Makefile.server

# Run c-xref over the ffmpeg source tree and verify it discovered an
# entry for every .c file under this test directory. Same shape as
# the original XrefMode test: count .c entries in the snapshot.
# Server-mode default (no -refnum) writes the whole snapshot as a
# single text file at .c-xref/db. File records are one line each,
# starting with <number>f and ending with the path — paths only
# appear in file records, so grep PWD then .c is unambiguous.
#
# Requires ~47M bytes of cxMemory.
#
# Cleanup removes any stale ffmpeg/.c-xrefrc that would intercept
# auto-detect before it walks up to test_ffmpeg/.c-xrefrc.

$(TEST):
	@if [ ! -d ffmpeg ] ; then \
		echo "Tests in '$(TEST)': not activated, run 'make init', 'make clean' to deactivate" ; \
		exit 0 ; \
	fi
	@rm -rf ffmpeg/.c-xref*
	@../run-with-progress $(SERVER_DRIVER) commands.input || exit 1
	@grep "$(PWD)" .c-xref/db | grep "\\.c" | wc -l | tr -d ' ' > output
	$(VERIFY)

init: ffmpeg

ffmpeg:
	tar zxf ffmpeg.tgz

# Re-run just the verification step against the existing .c-xref/db,
# without re-parsing ffmpeg (which takes ~25 minutes). Useful for
# iterating on the grep when the snapshot is already populated.
verify-only:
	@grep "$(PWD)" .c-xref/db | grep "\\.c" | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
