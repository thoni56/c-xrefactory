include ../Makefile.server

# Scenario: a project-wide push by name analyses again a file whose header changed
#
# Given main.c includes nothing, and user.c includes header.h and uses value
# And a push by name on value goes to header.h line 1
# When header.h is rewritten on disk, with value moved to line 2
# Then the next push by name analyses user.c again
# And goes to header.h line 2
#
# main.c, the request file, does not reach header.h, so only the project-wide
# analysis can bring user.c up to date.

$(TEST): .c-xrefrc
	@cp header_v1.h header.h
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
