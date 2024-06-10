include ../Makefile.boilerplate


ARGUMENTS = -xrefrc .c-xrefrc cgram_coverage.c -o $(TEST).log

$(TEST):
	rm -rf output $@.log CXrefs
	$(COMMAND)
	$(NO_TESTS)
	exit 0
