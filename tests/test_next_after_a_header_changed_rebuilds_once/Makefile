include ../Makefile.server

# A Next after a header changed on disk analyses what is out of date, once
#
# Given a.c, b.c and c.c include h.h, and a push in a.c has analysed them
# When h.h is changed on disk
# And Next is done twice in a.c
# Then the second Next analyses nothing
#
# The output is what the log says the last request builds.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	@cp h.h.v1 h.h
	@touch -t 200001010000 h.h a.c b.c c.c
	@-$(SERVER_DRIVER) commands.input --extra '-info -log=log' > output.tmp 2>&1
	@awk '/Siblings of/ {n = NR} {line[NR] = $$0} END {for (i = n + 1; i <= NR; i++) print line[i]}' log \
	    | grep -oE "Siblings: .*|Building .*|is out of date: .*" | sed "s|$(CURDIR)/||g" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
