include ../Makefile.boilerplate


$(TEST):
	@# First do the extract
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --buffer buffer --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1` -ex 'br refactory' -ex 'continue'
