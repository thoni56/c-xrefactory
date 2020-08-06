include ../Makefile.boilerplate

.SILENT:

$(TEST):
	../../src/c-xref -help > output.tmp
	grep -v debug output.tmp | grep -v trace > output
	$(VERIFY)
