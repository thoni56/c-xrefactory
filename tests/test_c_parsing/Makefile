include ../Makefile.server

# Smoke test: succeeds unless c-xref crashes parsing the C constructs
# in source.c. No symbol dump verification.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@((grep -E "ERROR|FATAL" run.stderr 2>/dev/null; grep -E "<error|<warning|<fatalError" run.stdout 2>/dev/null) || true) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
