include ../Makefile.boilerplate

CXREF = ../../src/c-xref
COMMAND = $(CXREF)  -xrefrc $(CURDIR)/.c-xrefrc -update -p $(CURDIR)/cexercise -trace -log=log

$(TEST): CXrefs
	touch cexercise/*
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: CXREF+=-trace
trace: $(TEST)

CXrefs: .c-xrefrc
	# Sources in subdir:
	sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	mv .c-xrefrc.tmp .c-xrefrc
	$(COMMAND) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

DEBUGGER=gdb
debug:
	touch cexercise/*
	$(DEBUGGER) --args $(COMMAND)
