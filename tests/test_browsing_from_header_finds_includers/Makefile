# Browsing a header's symbol finds the references in the files that include it
#
# The request file is the header itself, on a cold start with no snapshot. The
# compilation units that include it have never been parsed, so entry refresh
# Pass 3 has to parse them: it looks for headers the request file is about, and
# the request file being the header is one of those cases.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
