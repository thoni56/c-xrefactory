# Automated tests for pragma once
# Verification is just that it can parse "pragma once" and generate CXrefs

include ../Makefile.boilerplate

$(TEST):
	@rm -rf $(TEST).log CXrefs /tmp/c-xref*.log
	@../../src/c-xref include1.h include2.h -xrefrc .c-xrefrc -refs "$(CURDIR)/CXrefs" -o $(TEST).log
	@grep include1.h CXrefs/XFiles > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi
