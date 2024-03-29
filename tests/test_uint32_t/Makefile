include ../Makefile.boilerplate

# Test to see if uint32 is defined
#
# GCC 10 and up defines it using DEFINEs or typedefs which works
# automatically.  Some other compilers might not. As we now have
# introduced a possibility to discover which compiler is used, it is
# possible to inject the predefined symbols that C99 and later defines
# (like _Bool).  For now this only works for GCC, not for clang

# How c-xrefactory gets include paths:
#     LANG=C gcc -v -x c -o /dev/null /dev/null >out 2>&1

# and include standard defines:
#    gcc -E -dM - >out 2>&1

# And this should be what c-xrefactory actually sees
# gcc -E source.c


CXREF = ../../src/c-xref
ARGUMENTS = -create -xrefrc .c-xrefrc -p $(CURDIR) $(TRACE) uint32.c -errors -log=log

$(TEST): .c-xrefrc
	$(CXREF) $(ARGUMENTS) 2> output.tmp > out
	@$(NORMALIZE) output.tmp > output
	grep ERROR output | grep intptr_t; \
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
	$(DEBUGGER) $(CXREF) -ex 'br main' -ex 'run $(ARGUMENTS)'
