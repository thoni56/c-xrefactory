include ../Makefile.server

# A header left behind by a build is still stripped after a restart
#
# Given a.c and b.c both include h.h, and the editor holds a.c
# When h.h gets a line at the top on disk, a -getproject builds the held a.c
#   alone, and the server exits with a.c saved
# And a second session pushes in b.c
# Then f is listed only at its new line in h.h, not also at the old one

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer .c-xref
	@cp h.h.original h.h
	@-$(SERVER_DRIVER) commands_session1.input $(EXTRA1) > output.tmp 2>&1
	@-$(SERVER_DRIVER) commands_session2.input $(EXTRA2) >> output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =
