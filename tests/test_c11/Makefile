include ../Makefile.cli


ARGUMENTS = -create -errors source.c

$(TEST):
	$(COMMAND) > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	../../utils/cxref_reader | grep -v stdalign.h | grep -v stdnoreturn.h >> output
	$(VERIFY)

trace: ARGUMENTS += -debug -log=log
trace: $(TEST)

EXTRA_DEBUG =
