include ../Makefile.server

# Scenario: a dump answers also when a CU was rebuilt just before it
#
# Given a.c is held in the editor with unsaved changes, and parsed from them
# When the next request, a dump, no longer holds a.c, so a.c is rebuilt from disk
# Then the dump answers, and a.c has knowledge
#
# The rebuild resets the request's options, and the dump lost what it was asked for.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@touch -t 200001010000 a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
