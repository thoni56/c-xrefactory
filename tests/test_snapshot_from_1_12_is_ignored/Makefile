# A snapshot written by format 1.12.0 must be ignored, not read.
#
# 1.12.0 was kept while knowledge built from content the server edited, e.g. by a
# rename, stopped being written to the snapshot (97b3a7aa). Such a snapshot can hold
# a knowledge time that makes a CU look current while its references are not the
# ones on disk.
#
# Build a snapshot, set its version to 1.12.0, and move helper's definition to a line
# it is not on. If the snapshot is read anyway, the goto lands on the fabricated
# other.c:9, so the answer must be the one you get with no snapshot at all.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp
	@$(SERVER_DRIVER) query.input > no-snapshot.tmp
	@$(NORMALIZE) no-snapshot.tmp > no-snapshot.output
	@rm -rf .c-xref
	@$(SERVER_DRIVER) build.input > build.output
	@sed -i.bak -e 's/\(C-xrefactory \)[0-9.]*/\11.12.0/' -e 's/1u\([0-9]*\)f1l4cr/1u\1f9l4cr/' .c-xref/db
	@grep -aq 'C-xrefactory 1.12.0 ' .c-xref/db || { echo "version was not set to 1.12.0"; exit 1; }
	@grep -aq 'f9l4cr' .c-xref/db || { echo "definition position was not corrupted (snapshot encoding changed?)"; exit 1; }
	@$(SERVER_DRIVER) query.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff no-snapshot.output output > /dev/null || { echo "a 1.12.0 snapshot did not behave like no snapshot:"; diff no-snapshot.output output; exit 1; }
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
