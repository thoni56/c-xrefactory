include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -refactory -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -rfct-move-static-method -movetargetfile=$(CURDIR)/source/Target.java -rfct-param1=4 -olmark=59 -olcursor=59 -addimportdefault=0 $(CURDIR)/source/Source.java

$(TEST): CXrefs
	$(CXREF) $(ARGUMENTS) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

CXrefs: .c-xrefrc
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc sources -sourcepath $(CURDIR)/sources -o CXrefs.output

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output log

gdb debug:
	gdb --args $(CXREF) $(ARGUMENTS)
