include ../Makefile.boilerplate

# How c-xrefactory gets include paths:
#     LANG=C gcc -v -x c -o /dev/null /dev/null >out 2>&1

# and include standard defines:
#    gcc -E -dM - >out 2>&1

# And this should be what c-xrefactory actually sees
# gcc -E source.c

CXREF = ../../src/c-xref
COMMAND = $(CXREF) -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) uint32.c -log=log -o output.tmp

$(TEST): .c-xrefrc
	$(COMMAND)
	@$(NORMALIZE) output.tmp > output
	grep "error len" output ; \
	if [ $$? -eq 0 ] ; then \
		$(FAIL) ; \
	else \
		$(OK) ; \
	fi

.PHONY: trace
trace: TRACE=-trace
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

DEBUGGER = gdb
debug: TRACE = -trace
debug:
	$(DEBUGGER) --args $(COMMAND)
