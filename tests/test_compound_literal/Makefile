include ../Makefile.boilerplate

ARGUMENTS = test_sources/source.c -o output

$(TEST):
	@$(COMMAND)
	@# Is there a reference to the integer field on line with compound literal?
	@grep integer CXrefs/* | grep -q 16l ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi
