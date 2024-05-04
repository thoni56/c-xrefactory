include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGS = -xrefactory-II -refactory -xrefrc .c-xrefrc -p $PWD -rfct-extract-function -olmark=84 -olcursor=122 source.c

$(TEST): clean
	@$(CXREF) $(ARGS) $(EXTRAS) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRAS = -trace

log: EXTRAS = -log=log
log: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output

debug:
	gdb --args $(CXREF) $(ARGS)
