# Automated tests for <test case>

include ../Makefile.boilerplate

YACC = ../../byacc-1.9/yacc

# Note to self:
# The CXrefs file contains all references in a format that I have yet
# to completely decipher, but here are some details:
#
# A line for a file starts with an identification: '1541f'
# A line for a symbol starts with a field that includes the symbol name: '5/func'
# such a line also includes all references including file reference, line and char:
# '1541f9l17cr' means file 1541, line 9, character 17
#

$(TEST): $(YACC)
	@rm -rf $(TEST).log CXrefs
	@$(YACC) file.y
	@../../src/c-xref *.[cy] -xrefrc .c-xrefrc -refs "$(CURDIR)/CXrefs" -o $(TEST).log
	@# Get the "symbol" for the yacc-file from the CXrefs and use it
	@# to find the reference to func from the .y file
	@f=`grep process_yacc/file.y CXrefs/* | awk '{print $$1}'` ; \
	grep '	5/func	' CXrefs/* | grep '$f9l17cr' > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

$(YACC):
	make -C ../../byacc-1.9/
