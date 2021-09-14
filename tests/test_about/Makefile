include ../Makefile.boilerplate

.SILENT:

$(TEST):
	../../src/c-xref -about > output.tmp
	grep -v "This is" output.tmp | sed -e 's/git revision.*$$/git revision xxxxxx/' > output
	$(VERIFY)
