include ../Makefile.server

# Smoke test: succeeds unless c-xref crashes. Exists to drive cgram.y
# coverage; source intentionally minimal.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@: > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
