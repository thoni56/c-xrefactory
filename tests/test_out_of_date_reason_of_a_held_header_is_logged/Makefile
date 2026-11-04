# The log says when a CU is out of date since a header changed in the editor
#
# Given a.c and b.c both include h.h
# When a push in a.c finds both without knowledge, and a push with h.h held
#   with new content builds a.c and its sibling b.c
# Then the log names h.h as changed for both

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "'[^']*' is out of date: .*" log | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
