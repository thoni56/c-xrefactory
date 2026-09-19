include ../Makefile.cli


ARGUMENTS = -xref -debug -log=log -update

$(TEST): CXrefs
	touch cexercise/*
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)

CXrefs: .c-xrefrc
	# Sources in subdir:
	sed -e s:$(CURDIR)$$:$(CURDIR)/cexercise: .c-xrefrc > .c-xrefrc.tmp
	mv .c-xrefrc.tmp .c-xrefrc
	$(CXREF) -xref -create -o $(TEST).log
