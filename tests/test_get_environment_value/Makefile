include ../Makefile.boilerplate


$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF) -create test_source -o cxrefs.log

clean:
	rm -rf .c-xrefrc CXrefs output*

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -ex 'br searchStandardOptionsFileAndSectionForFile' -ex 'br errorMessage' -ex 'br answerEditAction' -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'continue'
