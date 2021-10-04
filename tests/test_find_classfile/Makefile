# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

CXREF = ../../src/c-xref
COMMAND =  -xrefrc .c-xrefrc src/x/Main.java

$(TEST): .c-xrefrc
	@$(CXREF) $(COMMAND) $(EXTRA) > output

trace: EXTRA = -trace -log=log
trace: $(TEST)

gdb:
	gdb --args $(CXREF) $(COMMAND)
