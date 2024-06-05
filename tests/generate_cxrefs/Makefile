include ../Makefile.boilerplate

COMMAND = ../../src/c-xref
ARGUMENTS = -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) -o output.tmp src

$(TEST): c_xrefrc_for_single_cxref_file
	rm CXrefs
	$(COMMAND) $(ARGUMENTS) | grep -v progress > output
	cat CXrefs >> output
	echo "<sync>" > expected
	cat expected.cxrefs >> expected
	$(VERIFY)

c_xrefrc_for_single_cxref_file: .c-xrefrc
ifeq ($(shell uname), Darwin)
	sed -i '' 's/refnum=10/refnum=1/' .c-xrefrc
else
	sed -i 's/refnum=10/refnum=1/' .c-xrefrc
endif
