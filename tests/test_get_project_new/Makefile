include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -olcxgetprojectname $(CURDIR)/index.c

$(TEST): CXrefs
	$(COMMAND) 2>1 > output.tmp
	$(NORMALIZE) output.tmp | grep -v -w progress > output
	$(VERIFY)

trace debug: EXTRA+=--extra="-trace -log=log"
trace: $(TEST)

CXrefs: .c-xrefrc
	$(CXREF) -create
