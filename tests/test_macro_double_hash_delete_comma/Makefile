include ../Makefile.boilerplate

# Generate full cross-reference info for systemd
# Will dump it but only compare if we run on "thoni64"
# since cross-references includes system files

# It requires 23854076 (~23M) bytes of cxMemory

ARGUMENTS = -create . -o systemd.log

all:
	rm -rf CXrefs output systemd.log log ; \
	../run-with-progress $(COMMAND) || exit 1 ; \
	../../utils/cxref_reader | grep -v :0:0 > output ; \
	if [ ! "$(shell uname -n)" = "thoni64" ] || diff -q output expected > /dev/null; then \
		$(OK); \
	else \
		$(FAIL); \
	fi; \


expected:
	sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc
	../../src/c-xref.orig -xrefrc .c-xrefrc -p $(PWD) -create .
	cxref_reader > expected

run:
	echo $(COMMAND)
	$(COMMAND)

UNIT_COMMAND = c-xref -xrefrc .c-xrefrc -p $$(PWD) stripped_unit.h

break:
	gdb -ex 'br expandMacroCall' -ex 'condition 1 (int)strcmp(macroSymbol.name, "inner_macro") == 0' -ex 'run' --args $(UNIT_COMMAND)

trace: ARGUMENTS += -trace
trace: all

init: systemd

systemd:
	tar zxf systemd.tgz
