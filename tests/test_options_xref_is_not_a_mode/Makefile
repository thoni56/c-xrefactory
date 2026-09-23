include ../Makefile.cli

# -xref used to select the legacy cross referencer. It is gone, so -xref is no
# longer a mode, and c-xref says that no mode was given.

$(TEST):
	@rm -rf CXrefs .c-xref
	@-$(CXREF_PROGRAM) -xref source.c > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=trace
trace: $(TEST)

EXTRA_DEBUG =
