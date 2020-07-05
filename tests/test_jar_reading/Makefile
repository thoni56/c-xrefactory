# No actual test except that it runs, for coverage mostly

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc abbot.jar

$(TEST):
	@$(COMMAND) > output
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
	rm -r .c-xrefrc CXrefs

gdb:
	gdb --args $(COMMAND)
