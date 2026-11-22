# Scenario: a header the request names before the scan is scanned for its includes
#
# Given p.h includes k.h, c.c includes p.h, and d.c includes only k.h
# When the first request, on a cold start, pushes K_TYPE in p.h
# Then the references include d.c, which shares k.h with p.h
#
# The request puts p.h in the file table before the scan, and the scan only
# reads a header that is new to the file table, so p.h's include of k.h is never
# found and d.c is not a sibling of the request.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@touch -t 200001010000 c.c d.c p.h k.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
