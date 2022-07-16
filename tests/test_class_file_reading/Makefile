include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -create -refnum=1 -log=log Completion2.class CNCtx.class

$(TEST): .c-xrefrc
	$(COMMAND) -o command.log
	# and compare some symbols to expected
	../../utils/cxref_reader | grep 'CNCtcx\|enumCount\|orbTracker' > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

clean:
	rm -rf CXrefs .c-xrefrc *.tmp

gdb:
	sudo gdb --args $(COMMAND)
