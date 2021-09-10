include ../Makefile.boilerplate

CXREF = ../../src/c-xref
CXREF_COMMAND = $(CXREF) -create -xrefrc .c-xrefrc -p $(CURDIR) -log=log source.c

$(TEST): .c-xrefrc
	$(CXREF_COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	grep "failed to open" log ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

GDB = gdb # May override for cgdb, lldb, ...
debug:
	$(GDB) --args $(CXREF_COMMAND)
