include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -refactory -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -rfct-move-static-method -movetargetfile=$(CURDIR)/source/target.c -rfct-param1=3 -olmark=33 -olcursor=33 $(CURDIR)/source/source.c

$(TEST):
	$(CXREF) $(ARGUMENTS) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output log

DEBUGGER = gdb
gdb debug:
	$(DEBUGGER) --args $(CXREF) $(ARGUMENTS)
