include ../Makefile.boilerplate


$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs: tweaked-c-xrefrc
	@$(CXREF) -create test_source -o cxrefs.log

tweaked-c-xrefrc: .c-xrefrc
	echo "  -set env ENV" >> .c-xrefrc


GDB_COMMANDS = -ex 'br searchStandardOptionsFileAndSectionForFile' -ex 'br errorMessage' -ex 'br answerEditAction' -ex 'continue'
