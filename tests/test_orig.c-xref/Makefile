include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	cat server-buffer >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)
