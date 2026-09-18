include ../Makefile.server


$(TEST): tweaked-c-xrefrc
	$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

tweaked-c-xrefrc: .c-xrefrc
	echo "  -set env ENV" >> .c-xrefrc


GDB_COMMANDS = -ex 'br errorMessage' -ex 'br fatalError' -ex 'br server.c:103' -ex 'br server.c:115' -ex 'cont'
