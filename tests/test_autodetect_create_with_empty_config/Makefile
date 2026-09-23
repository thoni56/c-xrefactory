# Auto-detection with nothing but a project section to go on
#
# There is no c-xrefrc.tpl here, so the common template is generated: a section
# naming this directory, and the -refs and -refnum that the server ignores for a
# detected project in favour of the convention path.
#
# The PUSH is what parses. Establishing the project only scans: source.c includes
# nothing, so no sibling shares a header with it and pass 3 has nothing to do,
# and the request file itself waits for an operation that needs it. The exit then
# writes the snapshot the dump reads.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output.tmp
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
