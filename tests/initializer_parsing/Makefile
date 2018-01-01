include ../Makefile.boilerplate

$(TEST):
	@rm -f $@.log Xrefs
	@printf "Tests in \"$@\": "
	@../../src/c-xref $@.c -xrefrc c-xrefrc -refs $(CURDIR)/C-Xrefs -o $@.log
# Check for the xrefs for 'E0'
	@grep "E0	4u2l2cr22u16l7cr18l5cr13cr23l3cr7cr13cr24l3cr25l7cr" C-Xrefs > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0
