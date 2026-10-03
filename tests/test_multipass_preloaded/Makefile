include ../Makefile.server

# Scenario: a preloaded file keeps the references of every pass
#
# Given a config with -pass1 -DPASS1 and -pass2 -DPASS2
# And source.c is preloaded with an edit that moves everything down a line
# When it is parsed, once per pass
# Then the table has pass1 from the first pass and pass2 from the second

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@touch source.preload   # a real client's preload is newer than the file
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
