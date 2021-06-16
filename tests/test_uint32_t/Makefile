include ../Makefile.boilerplate

# How c-xrefactory gets include paths:
#     LANG=C gcc -v -x c -o /dev/null /dev/null >out 2>&1

# and include standard defines:
#    gcc -E -dM - >out 2>&1

# And this should be what c-xrefactory actually sees
# gcc -E source.c

CXREF = ../../src/c-xref

$(TEST): .c-xrefrc
	$(CXREF) -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) -log=log source_not_working.c source_working.c -o output.tmp
	@$(NORMALIZE) output.tmp > output
	grep "error len" output

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp
