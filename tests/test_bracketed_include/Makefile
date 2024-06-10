include ../Makefile.boilerplate


ARGUMENTS = -create -log=log source.c

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	grep "failed to open" log ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp
