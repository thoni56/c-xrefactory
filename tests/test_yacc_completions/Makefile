include ../Makefile.server


$(TEST): sources-in-subdir
	@-$(SERVER_DRIVER) --cxref ../../src/c-xref commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

sources-in-subdir: .c-xrefrc
	@sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
