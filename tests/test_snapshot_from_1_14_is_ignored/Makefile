# A snapshot written by format 1.14.0 must be ignored, not read.
#
# 1.14.0 was kept while a header's references were removed all or nothing, with a mark
# for a header left behind. Such a snapshot can hold old references in a header beside
# the new ones, and since ADR-0035 nothing removes them unless the header changes.
#
# Build a snapshot, set its version to 1.14.0, and move helper's definition to a line
# it is not on. If the snapshot is read anyway, the goto lands on the fabricated
# other.c:9, so the answer must be the one you get with no snapshot at all.

include ../Makefile.server

OLD_VERSION = 1.14.0

$(TEST): .c-xrefrc
	@rm -rf output output.tmp
	@$(SERVER_DRIVER) query.input > no-snapshot.tmp
	@$(NORMALIZE) no-snapshot.tmp > no-snapshot.output
	@rm -rf .c-xref
	@$(SERVER_DRIVER) build.input > build.output
	@sed -i.bak -e 's/\(C-xrefactory \)[0-9.]*/\1$(OLD_VERSION)/' -e 's/1u\([0-9]*\)f1l4cr/1u\1f9l4cr/' .c-xref/db
	@grep -aq 'C-xrefactory $(OLD_VERSION) ' .c-xref/db || { echo "version was not set to $(OLD_VERSION)"; exit 1; }
	@grep -aq 'f9l4cr' .c-xref/db || { echo "definition position was not corrupted (snapshot encoding changed?)"; exit 1; }
	@$(SERVER_DRIVER) query.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff no-snapshot.output output > /dev/null || { echo "a $(OLD_VERSION) snapshot did not behave like no snapshot:"; diff no-snapshot.output output; exit 1; }
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
