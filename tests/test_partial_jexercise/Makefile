# Automated tests for parts of the java exercis, but without editor
# TODO There is something problematic with the first call (add parameter), "can't parse declaration"

include ../Makefile.boilerplate

CXREFRC=-xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
OUTPUT=>> output.tmp
CXREF=../../src/c-xref

$(TEST):
	echo "" > output.tmp
	$(CXREF) jexercise $(CXREFRC) -o $(TEST).log -sourcepath $(CURDIR)/jexercise $(OUTPUT)
	export SOURCEPATH=$(CURDIR)/jexercise ; $(CXREF) -refactory -rfct-add-param -olcxparnum=1 "-rfct-param1=int arg" -rfct-param2=0 $(CXREFRC) -olcursor=866 $(CURDIR)/jexercise/com/xrefactory/refactorings/MoveMethod.java $(OUTPUT)
	export SOURCEPATH=$(CURDIR)/jexercise ; $(CXREF) -refactory -rfct-del-param -olcxparnum=1 $(CXREFRC) -olcursor=866 $(CURDIR)/jexercise/com/xrefactory/refactorings/MoveMethod.java $(OUTPUT)
	#export SOURCEPATH=$(CURDIR)/jexercise ; $(CXREF) -refactory -rfct-move-param -olcxparnum=1 -olcxparnum2=1 $(CXREFRC) -olcursor=866 $(CURDIR)/jexercise/com/xrefactory/refactorings/MoveMethod.java $(OUTPUT)
	export SOURCEPATH=$(CURDIR)/jexercise ; $(CXREF) -refactory -rfct-rename -renameto=tototo $(CXREFRC) -olcursor=866 $(CURDIR)/jexercise/com/xrefactory/refactorings/MoveMethod.java $(OUTPUT)
	$(NORMALIZE) output.tmp | grep -v progress > output
	$(VERIFY)

clean:
	@rm -rf $@.log CXrefs output .c-xrefrc
