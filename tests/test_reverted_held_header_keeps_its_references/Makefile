include ../Makefile.server

# Scenario: a held header that is reverted keeps its references
#
# Given header.h declares shared, and a.c includes it
# And a push held header.h with changed content, so a.c was built from that
# When the next request no longer holds header.h, as after a revert in the editor
# And it is a push by name, which does not parse the request file
# Then shared is found at its declaration in header.h, as on disk
#
# When its buffer closes, header.h is left behind, so a.c is out of date although it is
# newer than header.h on disk. Building a.c puts the header's references back.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@touch -t 200001010000 header.h a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
