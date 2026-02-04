include ../Makefile.cli

$(TEST):
	-rm -rf .c-xref
	$(CXREF_PROGRAM) $(EXTRA) -create . > /dev/null
	../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace : EXTRA = -debug -log=trace
trace : $(TEST)
