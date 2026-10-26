# The log says why the knowledge of each CU is out of date
#
# Given a.c and b.c both include h.h
# When a push in a.c finds both without knowledge, and a push after h.h
#   changed on disk finds both built before it
# Then the log names no knowledge as the reason, and then h.h

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	@cp h.h.original h.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "'[^']*' is out of date: .*" log | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
