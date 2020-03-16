# No actual test except that it runs, for coverage mostly

include ../Makefile.boilerplate

$(TEST):
	@../../src/c-xref -xrefrc .c-xrefrc abbot.jar > output
	@if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

clean:
	rm .c-xrefrc CXrefs
