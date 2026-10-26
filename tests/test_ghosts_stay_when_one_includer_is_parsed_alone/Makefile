# A header is stripped even when its includers are built one request at a time
#
# Given a.c and b.c both include h.h, and the editor holds a.c
# When h.h gets a line at the top on disk, a -getproject builds the held a.c
#   alone, since it collects no siblings, and a push in b.c, with a.c saved,
#   then builds only b.c
# Then f is listed only at its new line in h.h, not also at the old one

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@cp h.h.original h.h
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
