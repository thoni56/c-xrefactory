include ../Makefile.cli


ARGUMENTS = -create -errors source.c

$(TEST):
	-$(COMMAND) 2>&1 | grep ERROR > output.tmp
	cat output.tmp | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: ARGUMENTS += -debug -log=log
trace: $(TEST)

EXTRA_DEBUG =
