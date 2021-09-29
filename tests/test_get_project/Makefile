include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST): CXrefs
	@sed -e"s+SRCDIR+$(realpath ../../src)+" commands.input > commands.tmp
	$(EDIT_SERVER_DRIVER) commands.tmp $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp | sed -e"s+$(realpath ../../src)+SRCDIR+" > output
	$(VERIFY)

trace: EXTRA+=--extra="-trace -log=log"
trace: $(TEST)

CXrefs:
	@sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" -e"s+SRCDIR+$(realpath ../../src)+" c-xref.project >> .c-xrefrc

clean:
	rm -rf .c-xrefrc CXrefs output*

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -ex 'br mainAnswerEditAction' -pid `pgrep c-xref | tail -1` -ex 'continue'

x:
	$(EDIT_SERVER_DRIVER) commands.input $(EXTRA)