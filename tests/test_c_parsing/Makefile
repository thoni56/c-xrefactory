include ../Makefile.boilerplate


ARGUMENTS = -xrefrc .c-xrefrc test_source/hello.c

# This test will always succeed unless c-xref fails
$(TEST): clean
	@$(COMMAND) -o output.tmp
	@$(NORMALIZE) output.tmp >> output
	@$(OK)
	@exit 0

clean:
	@-rm -rf CXrefs output.log output.tmp output
