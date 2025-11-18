# Running c-xref on itself to ensure that it doesn't crash and can show progress

include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@sed 's%  $(CURDIR)%  ../../src%' .c-xrefrc > .c-xrefrc.tmp
	@sed 's/=10/=1/' .c-xrefrc.tmp > .c-xrefrc
	@../run-with-progress ../../src/c-xref -p $(CURDIR) -create $(TRACE) -o c-xref.log
	if [ "`grep '<progress>' output | wc -l`" -gt 50 ] ; then \
	  $(OK) ; \
	  exit 0 ; \
	else \
	  $(FAIL) ; \
	  exit 1 ; \
	fi

trace: TRACE=-debug
trace: $(TEST)
