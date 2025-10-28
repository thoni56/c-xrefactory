include ../Makefile.boilerplate

ARGUMENTS = -create . -o test.log

all:
	$(COMMAND)
	../../utils/cxref_reader | grep -v :0:0 > output
	$(VERIFY)
