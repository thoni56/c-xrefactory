include ../Makefile.cli

# A mode has to be asked for. Without -server or -xref, c-xref says so and
# exits non-zero instead of quietly running the legacy cross referencer.

$(TEST):
	@-$(CXREF_PROGRAM) source.c > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=trace
trace: $(TEST)

EXTRA_DEBUG =
