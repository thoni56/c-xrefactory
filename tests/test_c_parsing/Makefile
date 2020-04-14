include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc test_source/hello.c

$(TEST): clean
	@$(COMMAND) -o output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp >> output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "diff output expected" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

clean:
	@-rm -rf CXrefs output.log output.tmp output buffer

gdb:
	gdb --args $(COMMAND)
