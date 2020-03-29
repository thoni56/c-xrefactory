# No actual test except that it runs, for coverage mostly

include ../Makefile.boilerplate

$(TEST):
	@../../src/c-xref -xrefrc .c-xrefrc abbot.jar > output
	@grep abbot/tester/JFileChooserTester CXrefs/X0* > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "Could not find abbot/tester/JFileChooserTester" ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

clean:
	rm .c-xrefrc CXrefs
