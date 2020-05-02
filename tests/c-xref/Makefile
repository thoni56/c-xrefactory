# Automated test for running c-xref on itself and see if generated
# cross references are the same

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed 's%  $(CURDIR)%  ../../src%' .c-xrefrc > .c-xrefrc.tmp
	@sed 's/=10/=1/' .c-xrefrc.tmp > .c-xrefrc
	@../run-c-xref -p $(CURDIR) -o c-xref.log
	@diff CXrefs expected > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

clean:
	rm -rf CXrefs c-xref.log out .c-xrefrc
