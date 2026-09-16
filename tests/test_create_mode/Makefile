# CreateMode: `c-xref -create2` should auto-discover the project from cwd, eagerly
# parse everything, write the .cx snapshot, and exit — without a server session.
# Driven incrementally: the first red is simply that `-create2` is an unknown
# option, which drives `options.serverOperation = OP_CREATE` in options.c.
include ../Makefile.cli

$(TEST):
	@rm -rf .c-xref
	@$(CXREF_PROGRAM) -create2 > output.tmp 2>&1 || true
	@../../utils/cxref_reader .c-xref/db >> output.tmp 2>&1 || true
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
