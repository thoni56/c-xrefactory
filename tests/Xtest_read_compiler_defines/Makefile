# Test that we don't get a warning for not finding stubs-32.h (on an
# x86_64 system). If we do that means that we haven't set all #define's
# that the system needs (usually set by the compiler)

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc

$(TEST):
	@$(CXREF) test_source > output.tmp
	@$(NORMALIZE) output.tmp > output
	@grep -q stubs-32.h output > /dev/null ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output* buffer
