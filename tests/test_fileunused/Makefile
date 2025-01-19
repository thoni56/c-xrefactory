include ../Makefile.boilerplate


$(TEST):
	@-$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
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

trace: EXTRA = --extra "-trace -log=log"
trace: $(TEST)

GDB_COMMANDS = -ex 'br processOptions' -ex 'br pushLocalUnusedSymbolsAction' -ex 'continue'
