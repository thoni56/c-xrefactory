include ../Makefile.boilerplate

$(TEST):
	@../../src/c-xref -p $(CURDIR) -xrefrc .c-xrefrc test_sources/source.c -o output
	@# Is there a reference to the integer field on line 14?
	@grep integer CXrefs/* | grep -q 14l ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi
