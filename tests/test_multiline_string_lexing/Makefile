include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc source.c

# This test will always succeed unless c-xref fails, it used to hang on unterminated macro
$(TEST): clean
	@$(COMMAND) -o output.tmp
	@$(NORMALIZE) output.tmp >> output
	@$(OK)
	@exit 0

clean:
	@-rm -rf CXrefs output.log output.tmp output

debug:
	gdb --args $(COMMAND) -log=log -trace
