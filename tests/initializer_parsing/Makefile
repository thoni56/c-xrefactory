include ../Makefile.boilerplate

$(TEST):
	@printf "Tests in \"$@\": "
	@rm -rf $@.log CXrefs
	@../../src/c-xref $@.c -xrefrc .c-xrefrc -refs $(CURDIR)/CXrefs -o $@.log
# Check for the xrefs for 'E0'
	@grep "E0" CXrefs/* | grep 'f2l2cr22u16l7cr18l5cr13cr23l3cr7cr13cr24l3cr25l7cr' > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0
