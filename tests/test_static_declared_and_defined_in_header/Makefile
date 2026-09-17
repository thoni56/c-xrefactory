# A static declared and defined in a header is one symbol
#
# setStaticFunctionLinkName names a static after the file it is in, but only on
# its definition. Any other usage, like the prototype in helper.h, is named
# after the file being parsed, which is the compilation unit main.c. If so, the
# prototype and the definition get different link names and become two symbols.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp
	@$(SERVER_DRIVER) commands.input > output.tmp
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
