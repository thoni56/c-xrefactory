include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II -refactory -rfct-move-function -commentmovinglevel=0 -movetargetfile=target.c -rfct-param1=3 -olcursor=32 source.c

$(TEST): .c-xrefrc
	@$(COMMAND) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

clean:
	rm -rf .c-xrefrc CXrefs output.log output.tmp

command:
	echo $(COMMAND)
