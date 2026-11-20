# Scenario: a pop keeps a held header
#
# Given a.c and b.c include h.h
# And a push in a.c holds h.h with new content
# When a pop holds h.h with the same content, as every request does
# And a push holds it again, after asking for the project as the client does
# Then neither the pop nor the push builds a.c or b.c, since h.h has not changed

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log .c-xref
	@touch -t 200001010000 a.c b.c h.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "Siblings: .*\|'[^']*' is out of date.*\|Building .*" log | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
