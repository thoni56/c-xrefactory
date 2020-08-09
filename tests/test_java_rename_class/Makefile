include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF) "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-encoding=european" "-refactory" "-user" "1" "-rfct-rename-class" "-renameto=RenameClass" "-olcursor=270" "-addimportdefault=0" "$(CURDIR)/jexercise/com/xrefactory/refactorings/RenameSymbol.java"

$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

gdb:
	gdb --args $(COMMAND)

CXrefs: .c-xrefrc
	@# Files in the jexercise subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

run:
	$(COMMAND) -o server-buffer
