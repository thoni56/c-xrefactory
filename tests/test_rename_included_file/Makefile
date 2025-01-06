include ../Makefile.boilerplate


ARGUMENTS = -refactory -rfct-rename-included-file -renameto=include2.h -olcursor=6 include.c

$(TEST): include.h
	$(CXREF) -create > output.tmp
	$(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

include.h:
	-rm include2.h*
	touch include.h

trace: EXTRA = \'-trace\'
trace: $(TEST)
