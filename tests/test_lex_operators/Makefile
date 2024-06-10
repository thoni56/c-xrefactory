include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefrc .c-xrefrc -create operators.c -o log
COMMAND = $(CXREF) $(ARGUMENTS)

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(OK)
	@exit 0
