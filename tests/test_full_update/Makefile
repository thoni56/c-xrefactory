include ../Makefile.boilerplate

CXREF = ../../src/c-xref
COMMAND = $(CXREF)  -xrefrc $(CURDIR)/.c-xrefrc -fullupdate -p $(CURDIR)

$(TEST): CXrefs
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
	@echo $(COMMAND)
	$(DEBUGGER) --args $(COMMAND)