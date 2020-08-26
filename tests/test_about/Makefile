include ../Makefile.boilerplate

.SILENT:

$(TEST):
	../../src/c-xref -about > output.tmp
	grep -v debug output.tmp | grep -v "This is" > output
	$(VERIFY)
