include ../Makefile.cli

$(TEST):
	@rm -rf .c-xref
	@$(CXREF_PROGRAM) -create source.c > output.tmp 2>&1 || true
	ls .c-xref/db >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
