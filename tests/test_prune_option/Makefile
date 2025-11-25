include ../Makefile.boilerplate

# Verify that pruned directories are not included

ARGUMENTS = -xrefactory-II -create -errors $(TRACE) -log=log -o output.tmp -prune pruned

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp
	grep $(PWD) CXrefs/XFiles | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: TRACE = -debug
trace: $(TEST)
