include ../Makefile.cli

# A mode has to be asked for. Without one, c-xref says so and exits non-zero
# instead of quietly running the legacy cross referencer.
#
# The message names the two modes worth asking for. -lsp is one of them even
# though it is not a value of options.mode: main() checks want_lsp_server
# before anything else. -xref is not named because it is gone.

$(TEST):
	@-$(CXREF_PROGRAM) source.c > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=trace
trace: $(TEST)

EXTRA_DEBUG =
