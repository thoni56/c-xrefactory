include ../Makefile.cli

$(TEST):
	@-rm -r .c-xrefdb 2> /dev/null
	@../../src/c-xref -create .
	@if [ -d .c-xrefdb ] ; then \
		$(OK) ; \
	else \
		$(FAIL) ; \
	fi
