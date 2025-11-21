include ../Makefile.boilerplate


ARGUMENTS = test_source/hello.c

# This test will always succeed unless c-xref fails
$(TEST):
	@$(COMMAND) -o output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)
