include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-organize-includes -olcursor=$(OFFSET) source.c

$(TEST): .c-xrefrc
	touch *.c
	-@$(MAKE) --no-print-directory OFFSET=0 run-once > output.tmp
	-@$(MAKE) --no-print-directory OFFSET=67 run-once >> output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	$(VERIFY)

run-once:
	@$(COMMAND)

trace: EXTRA = -debug -log=log -lexemtrace
trace: $(TEST)
