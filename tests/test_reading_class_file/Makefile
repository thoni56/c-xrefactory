include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -log=log -trace Completion2.class

$(TEST):
	$(COMMAND) > output.tmp
	@grep classfilereader log | sed -E 's/[[:digit:]]{4}-[[:digit:]]{2}-[[:digit:]]{2} [[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}/0000-00-00 00:00:00/' >> output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp > output
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

gdb:
	sudo gdb --args $(COMMAND)
