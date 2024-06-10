include ../Makefile.boilerplate

ARGUMENTS = initializer_parsing.c -xrefrc .c-xrefrc -refs $(CURDIR)/CXrefs -o output

$(TEST):
	@$(COMMAND)
	@# Check in the xrefs for 'E0' - which means?
	@grep "E0" CXrefs/* | grep 'f2l2cr22u16l7cr18l5cr13cr23l3cr7cr13cr24l3cr25l7cr' > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	fi
	@$(OK)
	@exit 0

clean:
	@rm -rf $@.log CXrefs .c-xrefrc *output
