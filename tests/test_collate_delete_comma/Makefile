include ../Makefile.cli

ARGUMENTS = -create . -o command.log

all:
	$(COMMAND)
	../../utils/cxref_reader | grep -v :0:0 > output
	$(VERIFY)
