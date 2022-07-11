include ../Makefile.boilerplate

.SILENT:

$(TEST):
	../../src/c-xref -no-stdoptions -about > output.tmp
	grep -v "Compiled at" output.tmp | sed -e 's/git revision.*$$/git revision xxxxxx/' > output
	$(VERIFY)
