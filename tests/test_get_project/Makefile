include ../Makefile.server


$(TEST): .c-xrefrc
	$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA+=--extra="-debug -log=log"
trace: $(TEST)


GDB_COMMANDS = -ex 'br mainEditServer' -ex 'continue'
