# Automated tests for <test case>

include ../Makefile.cli

YACC = ../../byacc-1.9/yacc

$(TEST): $(YACC) .c-xrefrc
	@rm -rf $(TEST).log CXrefs
	@$(YACC) file.y
	@$(COMMAND) *.[cy] -o $(TEST).log
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
