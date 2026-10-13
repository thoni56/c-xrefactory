include ../Makefile.server

# Scenario: rename reaches a file through a header the request includes indirectly
#
# Given main.c includes a.h, a.h includes decl.h, and decl.h declares counter
# And z.c includes only decl.h, defines counter and uses it, and has never been analysed
# When counter is renamed to total from main.c
# Then z.c is renamed too
#
# The entry refresh collects what to build from the headers main.c includes
# directly (collectSiblingCUs), so only from a.h. decl.h is in main.c's include
# closure, but who else includes it is never asked, and z.c is not built.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
