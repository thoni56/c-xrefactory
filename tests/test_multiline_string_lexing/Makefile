include ../Makefile.boilerplate

ARGUMENTS = source.c

# This test will always succeed unless c-xref fails, it used to hang on unterminated macro
$(TEST):
	@$(COMMAND) -o output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)
