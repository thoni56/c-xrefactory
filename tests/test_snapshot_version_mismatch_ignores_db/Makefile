# A snapshot written by another format version must be ignored, not read.
#
# Build a snapshot with both files parsed, then corrupt it twice over: bump the
# format version, and move helper's definition to a line it is not on. A
# mismatched version has to make the whole snapshot be ignored, so the answer
# must be the one you get with no snapshot at all. If the snapshot is read
# anyway, the goto lands on the fabricated other.c:9.
#
# Both corruptions are checked, so the test cannot pass by failing to corrupt,
# and the answer is compared against a run with no snapshot as well as against
# `expected`.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp
	@$(SERVER_DRIVER) query.input > no-snapshot.tmp
	@$(NORMALIZE) no-snapshot.tmp > no-snapshot.output
	@rm -rf .c-xref
	@$(SERVER_DRIVER) build.input > build.output
	@sed -i.bak -e 's/\(C-xrefactory \)[0-9]/\19/' -e 's/1u\([0-9]*\)f1l4cr/1u\1f9l4cr/' .c-xref/db
	@grep -aq 'C-xrefactory 9' .c-xref/db || { echo "version was not corrupted (has it reached 9.x?)"; exit 1; }
	@grep -aq 'f9l4cr' .c-xref/db || { echo "definition position was not corrupted (snapshot encoding changed?)"; exit 1; }
	@$(SERVER_DRIVER) query.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@diff no-snapshot.output output > /dev/null || { echo "a wrong-version snapshot did not behave like no snapshot:"; diff no-snapshot.output output; exit 1; }
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
