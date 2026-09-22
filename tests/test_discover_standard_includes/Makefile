include ../Makefile.server

# The compiler's standard include directories are discovered, so <stdbool.h> is
# found and parsed: `bool`, `true` and `false` are defined nowhere else, and can
# only be in the table if the header was opened.
#
# Their positions are not asserted. They live in the compiler's own stdbool.h and
# differ between machines and compiler versions; the names do not.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@../../utils/cxref_reader .c-xref/db | grep -xE 'bool|true|false' | sort > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
