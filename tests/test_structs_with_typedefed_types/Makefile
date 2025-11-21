include ../Makefile.boilerplate

ARGUMENTS = struct.c

# This test will always succeed unless c-xref fails
$(TEST):
	$(COMMAND) -o output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
