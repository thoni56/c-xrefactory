include ../Makefile.boilerplate


ARGUMENTS = xrefrc .c-xrefrc source.c -o log

$(TEST):
	rm -rf output $@.log CXrefs Xrefs
	$(COMMAND)
	ls CXrefs | wc -l > output
	$(VERIFY)
