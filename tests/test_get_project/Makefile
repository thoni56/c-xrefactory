include ../Makefile.boilerplate


$(TEST): CXrefs
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace debug: EXTRA+=--extra="-trace -log=log"
trace: $(TEST)

CXrefs:
	@sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" -e"s+SRCDIR+$(realpath ../../src)+" c-xref.project >> .c-xrefrc

debug:
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -ex 'br mainEditServer' -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'` -ex 'continue'
