include ../Makefile.boilerplate

CXREF = ../../src/c-xref

$(TEST): clean
	@# First do the extract
	@-$(EDIT_SERVER_DRIVER) commands1.input $(CURDIR) > output1.tmp
	@$(NORMALIZE) output1.tmp > output
	@# Then a rename - as the editor would
	@-$(EDIT_SERVER_DRIVER) commands2.input $(CURDIR) > output2.tmp
	@$(NORMALIZE) output2.tmp >> output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "diff output expected" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

clean:
	@-rm -rf CXrefs output.log output.tmp output

gdb:
	@$(EDIT_SERVER_DRIVER) commands1.input "$(CURDIR)" buffer1 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1` -ex 'br mainRefactory' -ex 'continue'
