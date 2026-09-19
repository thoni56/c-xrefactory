include ../Makefile.server

# The compiler's standard defines are discovered, so `#ifdef __STDC_VERSION__`
# takes the true branch: `a` is in the dump and `b` is not.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
