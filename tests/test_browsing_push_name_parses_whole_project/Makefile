# Push by name finds a symbol in files not yet parsed
#
# Pushing a name has no position, so nothing in the include graph says which
# files can define it; like -search, it is project-wide and needs every CU
# parsed. main.c, the request file, includes nothing, so on a cold start the
# only way to find helper is to parse helper.c and other.c.
#
# expected assumes push by name asks before parsing everything, the way -search
# does ("N of M compilation units need reparsing"), answered by -continue.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
