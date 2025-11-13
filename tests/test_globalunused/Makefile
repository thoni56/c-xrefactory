include ../Makefile.boilerplate


$(TEST): CXrefs
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA=--extra "-debug"
trace: $(TEST)

ARGUMENTS = -create

CXrefs: .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

GDB_COMMANDS = -ex 'br scanForGlobalUnused' -ex 'br setSelectedVisibleItems' -ex 'continue'
