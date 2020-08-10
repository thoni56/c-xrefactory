include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -create -refnum=1 -log=log Completion2.class CNCtx.class

$(TEST):
	# Put all references in a single file
	sed -e '/refnum/d' .c-xrefrc > .c-xrefrc.tmp ; mv .c-xrefrc.tmp .c-xrefrc
	$(COMMAND) > output.tmp
	# and compare it to expected
	$(NORMALIZE) CXrefs > output
	cat output # To catch the output from Travis build - just temporary
	$(VERIFY)

clean:
	rm -rf CXrefs .c-xrefrc *.tmp

gdb:
	sudo gdb --args $(COMMAND)
