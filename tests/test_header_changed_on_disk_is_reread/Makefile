include ../Makefile.server

# Scenario: a header changed on disk is read again
#
# Given source.c uses LIMIT from header.h, which nobody has open in an editor
# And a push on the use goes to the definition on line 1
# When header.h is rewritten on disk, with LIMIT moved to line 3, and no preload
# Then the next push goes to line 3
#
# It works because the push parses source.c again, and buffers the client does not
# hold are closed after each request, so header.h is read from disk again.

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@cp header_v1.h header.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
