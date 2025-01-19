include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -olcxgetprojectname index.c

$(TEST): CXrefs
	$(COMMAND) 2>1 > output.tmp
	$(NORMALIZE) output.tmp | grep -v -w progress > output
	$(VERIFY)

trace: EXTRA+=--extra="-trace -log=log"
trace: $(TEST)

CXrefs: .c-xrefrc
	$(CXREF) -create
