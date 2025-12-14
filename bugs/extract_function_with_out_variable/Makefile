include ../../tests/Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -refactory -xrefrc .c-xrefrc -p $PWD -rfct-extract-function -olmark=83 -olcursor=121 source.c

$(TEST):
	@$(COMMAND) $(EXTRAS) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRAS = -trace

log: EXTRAS = -log=log
log: $(TEST)
