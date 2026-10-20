include ../Makefile.server

# Multi-pass parsing with pass-specific defines: the config gives -pass1 -DPASS1
# and -pass2 -DPASS2, and symbols from both #ifdef branches must end up in one
# table, without spillover. The syntax error planted in the PASS2 branch is
# reported with its position.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer run.stdout
	@$(SERVER_DRIVER) commands.input $(EXTRA) > run.stdout
	@../../utils/cxref_reader .c-xref/db > output
	@(grep -o "<log [^>]*>.*" run.stdout; true) | $(NORMALIZE) >> output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
