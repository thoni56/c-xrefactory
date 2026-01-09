include ../Makefile.server


$(TEST): CXrefs
	$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

CXrefs: tweaked-c-xrefrc
	@$(CXREF) -create test_source -o cxrefs.log

tweaked-c-xrefrc: .c-xrefrc
	echo "  -set env ENV" >> .c-xrefrc


GDB_COMMANDS = -ex 'br errorMessage' -ex 'br fatalError' -ex 'br server.c:103' -ex 'br server.c:115' -ex 'cont'
