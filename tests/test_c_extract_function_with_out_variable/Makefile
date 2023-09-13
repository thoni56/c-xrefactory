include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST): clean
	@$(CXREF) -xrefactory-II -refactory -xrefrc .c-xrefrc -p $PWD -rfct-extract-method -olmark=54 -olcursor=100 source.c -trace $(EXTRAS) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

log: VERIFY = -log=log
log: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output

gdb:
	gdb --args $(CXREF) -xrefactory-II -refactory -xrefrc .c-xrefrc -p $PWD -rfct-extract-method -olmark=54 -olcursor=100 source.c
