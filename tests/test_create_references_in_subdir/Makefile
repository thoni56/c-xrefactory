include ../Makefile.boilerplate

COMMAND=../../src/c-xref -xrefactory-II -create -errors -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR) -o output

$(TEST): .c-xrefrc
	@sed -e 's:/CXrefs$$:/CXrefs/subdir:' .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
	@-$(COMMAND) > stdout
	@if [ -d CXrefs/subdir ] ; then \
		$(OK) ; \
	else \
		$(FAIL) ; \
	fi

debug: DEBUGGER=gdb
debug:
	$(DEBUGGER) --args $(COMMAND)

clean:
	rm -rf .c-xrefrc CXrefs
