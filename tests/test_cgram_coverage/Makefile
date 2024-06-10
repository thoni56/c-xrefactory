include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc cgram_coverage.c -o $(TEST).log

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	rm -rf output $@.log CXrefs
	$(COMMAND)
	$(NO_TESTS)
	exit 0
