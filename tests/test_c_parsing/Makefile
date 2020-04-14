include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc test_source/hello.c

# This test will always succeed unless it c-xref fails
$(TEST): clean
	@$(COMMAND) -o output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp >> output
	@$(OK)
	@exit 0

clean:
	@-rm -rf CXrefs output.log output.tmp output buffer

gdb:
	gdb --args $(COMMAND)
