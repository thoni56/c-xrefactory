include ../Makefile.server

$(TEST): .c-xrefrc
	-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	cat server-buffer >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
