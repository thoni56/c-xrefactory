include ../Makefile.cli


ARGUMENTS = -create .

$(TEST):
	echo "-optinclude more_options" >> .c-xrefrc
	$(COMMAND) > output.tmp 2>&1
	ls CXrefs | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: ARGUMENTS += -trace -log=trace
trace: $(TEST)

EXTRA_DEBUG =
