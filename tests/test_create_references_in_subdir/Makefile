include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -create -errors -o output

$(TEST): .c-xrefrc
	@sed -e 's:/CXrefs$$:/CXrefs/subdir:' .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@-$(COMMAND) > stdout
	@if [ -d CXrefs/subdir ] ; then \
		$(OK) ; \
	else \
		$(FAIL) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs
