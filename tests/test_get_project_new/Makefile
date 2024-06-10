include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -olcxgetprojectname $(CURDIR)/index.c

$(TEST): CXrefs
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp | grep -v -w progress > output
	$(VERIFY)

trace debug: EXTRA+=--extra="-trace -log=log"
trace: $(TEST)

CXrefs: .c-xrefrc
	echo CXREF
	$(CXREF) -create

clean:
	rm -rf CXrefs output* .c-xrefrc
