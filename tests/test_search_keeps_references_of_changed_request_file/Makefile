include ../Makefile.server

# Scenario: a search keeps the references of a request file changed on disk
#
# Given a.c defines f and calls it, and has been parsed
# When a.c is rewritten on disk, with f one line down
# And f is searched for, with a.c as the request file
# Then the search finds f in a.c
#
# A search does not parse the request file, so it must not remove its references either.

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp a.v1 a.c
	@touch -t 200001010000 a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
