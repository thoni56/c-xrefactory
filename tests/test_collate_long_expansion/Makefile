include ../Makefile.server

# Smoke test only: source has FLAG_STRING-style macros (from ffmpeg)
# whose expansion is large and noisy. Skip the symbol dump and just
# verify the parser doesn't crash on the long expansion.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@: > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
