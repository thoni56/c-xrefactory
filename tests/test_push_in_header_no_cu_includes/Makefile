include ../Makefile.server

# Scenario: a push in a header that no compilation unit includes says so
#
# Given user.c includes header.h
# And f is pushed on in header.h
# When user.c is rewritten on disk without the include
# And f is pushed on in header.h again
# Then the answer says that no compilation unit includes header.h
#
# Today header.h is parsed on its own as the request file, and its references come
# back without any context.

$(TEST): .c-xrefrc
	@cp user.c.v1 user.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
