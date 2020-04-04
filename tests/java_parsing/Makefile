# Automated tests for parsing java
#
# Starting from the jexercise but adding more constructs
# that we want to cover with tests

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc src -o $(TEST).output

$(TEST):
	@rm -rf $(TEST).output CXrefs
	@$(COMMAND)
	@grep -i error $(TEST).output ; \
	if [ $$? -ne 1 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

gdb:
	gdb --args $(COMMAND)

clean:
	@rm -rf *.output *.log .c-xrefrc CXrefs
