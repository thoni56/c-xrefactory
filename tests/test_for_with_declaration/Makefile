include ../Makefile.boilerplate


$(TEST): .c-xrefrc
	$(COMMAND) > out
	@$(NORMALIZE) output.tmp > output
	grep "error len" output ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp
