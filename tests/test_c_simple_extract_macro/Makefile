include ../Makefile.boilerplate


$(TEST):
	@# First do the extract
	@-$(EDIT_SERVER_DRIVER) commands1.input $(EXTRA) > output1.tmp
	@$(NORMALIZE) output1.tmp > output
	@# Then a rename - as the editor would
	@-$(EDIT_SERVER_DRIVER) commands2.input $(EXTRA) > output2.tmp
	@$(NORMALIZE) output2.tmp >> output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

GDB_COMMANDS = -ex 'br refactory' -ex 'continue'
