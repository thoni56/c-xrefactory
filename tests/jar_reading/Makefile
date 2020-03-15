# No actual test except that it runs, for coverage mostly

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@../../src/c-xref -xrefrc .c-xrefrc abbot.jar > output
	@if [ $$? -ne 0 ] ; then \
		echo "Tests in '$(TEST)': Failed" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		echo "Tests in '$(TEST)': Ok" ; \
		exit 0; \
	fi

clean:
	rm .c-xrefrc CXrefs
