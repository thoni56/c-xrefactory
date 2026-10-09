include ../Makefile.server

# Scenario: asking for the project parses nothing
#
# Given a.c and b.c both include f.h, and b.c defines g and calls f
# When the client asks for the project of a.c, and the server exits
# Then the snapshot holds the files and their includes from the scan
# And nothing that only a parse would find, like g or the call of f in b.c

$(TEST): .c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@../../utils/cxref_reader .c-xref/db >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
