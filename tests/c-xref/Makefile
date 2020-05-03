# Just running c-xref on itself to ensure that it doesn't crash

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed 's%  $(CURDIR)%  ../../src%' .c-xrefrc > .c-xrefrc.tmp
	@sed 's/=10/=1/' .c-xrefrc.tmp > .c-xrefrc
	@printf "  -I../../src" >> .c-xrefrc
	@../run-c-xref -p $(CURDIR) -o c-xref.log
	@$(OK)
	@exit 0

clean:
	rm -rf CXrefs c-xref.log out .c-xrefrc

gdb:
	gdb --args ../../src/c-xref -create -xrefrc .c-xrefrc -p $(CURDIR)
