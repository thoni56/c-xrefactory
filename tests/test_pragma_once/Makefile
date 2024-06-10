include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@rm -rf $(TEST).log CXrefs /tmp/c-xref*.log
	@$(CXREF) include1.h include2.h -o $(TEST).log
	@grep include1.h CXrefs/XFiles > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi
