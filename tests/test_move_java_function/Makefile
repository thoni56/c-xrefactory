include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -refactory -xrefrc .c-xrefrc -p $(CURDIR) -rfct-move-static-method -movetargetfile=$(CURDIR)/source/Target.java -rfct-param1=4 -olmark=59 -olcursor=59 $(CURDIR)/source/Source.java

$(TEST): .c-xrefrc
	-rm -rf CXrefs
	../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc -sourcepath sources -create source > CXrefs.output
	$(CXREF) $(ARGUMENTS) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

clean:
	@-rm -rf .c-xrefrc CXrefs output.tmp output log

gdb debug:
	gdb --args $(CXREF) $(ARGUMENTS)
