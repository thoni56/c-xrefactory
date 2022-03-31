include ../Makefile.boilerplate

.SILENT:

$(TEST):
	../../src/c-xref -no-stdoptions -about > output.tmp
	grep -v "This is" output.tmp | sed -e 's/git revision.*$$/git revision xxxxxx/' > output
	$(VERIFY)
