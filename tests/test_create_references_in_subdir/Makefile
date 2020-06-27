include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed -e 's:/CXrefs$$:/CXrefs/subdir:' .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@-../../src/c-xref -xrefactory-II -create -errors -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -o output > stdout
	@if [ -d CXrefs/subdir ] ; then \
		$(OK) ; \
	else \
		$(FAIL) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs
