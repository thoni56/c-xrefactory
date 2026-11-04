# Scenario: a header let go is current again, although it is included through another
#
# Given a.c and b.c include upper.h, which includes inner.h
# And a push held inner.h with new content, so both were built from that
# When the next push no longer holds inner.h, which changes it, and builds both
# Then a third push finds nothing out of date

include ../Makefile.server

# Which CU is asked first follows file numbers, i.e. path hashes, so the lines of
# each request, which ends with its Siblings line, are sorted
SORT_EACH_REQUEST = awk '/^Siblings:/ { close("LC_ALL=C sort"); print; fflush(); next } { print | "LC_ALL=C sort" }'

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	@touch -t 200001010000 a.c b.c upper.h inner.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@grep -o "Siblings: .*\|'[^']*' is out of date: .*" log | $(NORMALIZE) | $(SORT_EACH_REQUEST) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
