include ../Makefile.server

# Smoke test: succeeds unless c-xref crashes parsing the C constructs
# in source.c. No symbol dump verification.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@: > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
