include ../Makefile.cli


ARGUMENTS = cgram_coverage.c -o $(TEST).log

$(TEST):
	rm -rf output $@.log CXrefs
	$(COMMAND)
	$(NO_TESTS)
	exit 0
