include ../Makefile.cli

.SILENT:

$(TEST):
	../../src/c-xref -about > output.tmp
	grep -v "Compiled at" output.tmp | sed -e 's/C-xrefactory version .*$$/C-xrefactory version X.Y.Z/' | sed -e 's/git revision .*$$/git revision xxxxxx/' > output
	$(VERIFY)
