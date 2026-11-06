# Scenario: a CU above a changed header is among the includers of a header below it
#
# Given a.c includes mid.h, which includes leaf.h and defines CALL as f
# And a.c expands CALL, and leaf.h declares f
# And f has been pushed on in leaf.h, so a.c is built
# When mid.h is rewritten on disk with a line inserted above CALL
# And f is pushed on in leaf.h again
# Then the use of f in the body of CALL, which only a.c puts there, is among its references
#
# The change removes mid.h's references, and its include of leaf.h with them, so
# a.c is reached from leaf.h only through the include graph taken before the
# removal. The sleeps keep each build more than the coarse clock's 10 ms after
# the change before it.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp mid.h.before mid.h
	@touch -t 200001010000 a.c leaf.h mid.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
