include ../Makefile.server

# Test to see if uint32 is defined
#
# GCC 10 and up defines it using DEFINEs or typedefs which works
# automatically.  Some other compilers might not. As we now have
# introduced a possibility to discover which compiler is used, it is
# possible to inject the predefined symbols that C99 and later defines
# (like _Bool).  For now this only works for GCC, not for clang

# How c-xrefactory gets include paths:
#     LANG=C gcc -v -x c -o /dev/null /dev/null >out 2>&1

# and include standard defines:
#    gcc -E -dM - >out 2>&1

# And this should be what c-xrefactory actually sees
# gcc -E source.c

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db | grep uint32_t > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
