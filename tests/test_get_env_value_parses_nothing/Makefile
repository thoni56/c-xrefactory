include ../Makefile.server

# Scenario: -get parses nothing
#
# Given a.c and b.c both include f.h
# And the config sets a variable built from the request file's name
# When the client asks for the variable with a.c as the request file
# Then the answer needs only the file's name, not its content
# And the snapshot holds the files and their includes from the scan, nothing parsed

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@../../utils/cxref_reader .c-xref/db >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
