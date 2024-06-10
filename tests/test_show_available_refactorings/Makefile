include ../Makefile.boilerplate


$(TEST):
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA=--extra "-trace"
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

debug:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'br server' -ex 'continue'
