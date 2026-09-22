include ../Makefile.server

# An options file pulled in with -optinclude is read, and its options take
# effect on the parse: -DFROM_OPTINCLUDE decides which branch of the #ifdef
# exists. The alternative branch's symbol is what a missed include would leave
# behind, so the dump tells the two apart.
#
# The -optinclude line is in this test's own c-xrefrc.tpl, not appended to the
# generated .c-xrefrc by the recipe: appending accumulated one more copy of the
# line on every run, and left the option in place even when the recipe dropped it.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
