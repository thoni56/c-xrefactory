include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -refnum=1 -log=log Completion2.class CNCtx.class

$(TEST):
	-rm -r CXrefs
	# Put all references in a single file
	sed -e '/refnum/d' .c-xrefrc > .c-xrefrc.tmp ; mv .c-xrefrc.tmp .c-xrefrc
	$(COMMAND) > output.tmp
	# and compare it to expected
	$(NORMALIZE) CXrefs > output
	$(VERIFY)

clean:
	rm -rf CXrefs .c-xrefrc *.tmp

gdb:
	sudo gdb --args $(COMMAND)
