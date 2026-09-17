# Reference and menu filters on a global used across files
#
# counter is declared in globals.h, defined in globals.c, and in main.c written,
# has its address taken, and read. Each -olcxfilter level should drop the next
# kind of reference: reads, then address-taking, then everything but
# definitions. a.c has a static counter, a different symbol with the same name,
# which the menu shows at -olcxmenufilter=0 but not at 1. The local counter in
# b.c must never appear.
#
# a.c is browsed (push, pop) first so that it is parsed. Nothing connects it to
# main.c through includes, and on cold start the menu only knows symbols from
# parsed files; whether it should know all of them is test_browsing_push_name_parses_whole_project.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
