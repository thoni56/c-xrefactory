include ../Makefile.boilerplate

CXREF = ../../src/c-xref
COMMAND = $(CXREF) -xrefactory-II -refactory -xrefrc .c-xrefrc -p $(CURDIR) -user 1 -rfct-extract-method -olmark=413 -olcursor=327 jexercise/com/xrefactory/refactorings/ExtractMethod.java -o output.tmp > command_output

$(TEST):
	@-$(COMMAND)
	@$(NORMALIZE) output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "diff output expected" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

clean:
	@-rm -rf .c-xrefrc CXrefs output.log output.tmp output *~ log

gdb:
	@gdb --args $(COMMAND)
