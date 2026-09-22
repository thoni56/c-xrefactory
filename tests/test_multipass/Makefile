include ../Makefile.server

# Multi-pass parsing with pass-specific defines: the config gives -pass1 -DPASS1
# and -pass2 -DPASS2, and symbols from both #ifdef branches must end up in one
# table, without spillover.
#
# The other half of the original XrefMode test, that the syntax error planted in
# the PASS2 branch is reported with file and line, is not here: formatMessage()
# drops the position in server mode.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
