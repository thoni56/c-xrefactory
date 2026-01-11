include ../Makefile.cli


ARGUMENTS = -refactory -rfct-del-param -olcxparnum=2 -olcursor=75 source.c -preload source.c source.preload

$(TEST):
	$(CXREF) -create > output.tmp
	$(COMMAND) >> output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS += -trace -log=log
trace: $(TEST)

EXTRA_DEBUG =
