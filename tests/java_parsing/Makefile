# Automated tests for parsing java
#
# Starting from the jexercise but adding more constructs
# that we want to cover with tests

include ../Makefile.boilerplate

TEST = java_example

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc src -o $(TEST).log

$(TEST):
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@grep -i error $(TEST).log ; \
	if [ $$? -ne 1 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

gdb:
	gdb --args $(COMMAND)
