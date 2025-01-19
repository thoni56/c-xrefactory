include ../Makefile.boilerplate


$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra \'-trace\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@# Sources in subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	@mv .c-xrefrc.tmp .c-xrefrc
