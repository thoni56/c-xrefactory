include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)

$(TEST): .c-xrefrc
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

gdb:
	gdb --args `cat commands.input`

run:
	$(COMMAND) -o server-buffer

clean:
	rm -rf CXrefs .c-xrefrc
