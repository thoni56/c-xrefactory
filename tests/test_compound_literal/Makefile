include ../Makefile.boilerplate

ARGUMENTS = -p $(CURDIR) -xrefrc .c-xrefrc test_sources/source.c -o output

$(TEST):
	@$(COMMAND)
	@# Is there a reference to the integer field on line with compound literal?
	@grep integer CXrefs/* | grep -q 16l ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi
