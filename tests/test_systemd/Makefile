include ../Makefile.boilerplate

# Generate full cross-reference info for systemd
# Will dump it but only compare if we run on "thoni64"
# since cross-references includes system files

# It requires 23854076 (~23M) bytes of cxMemory

ARGUMENTS = -create -o systemd.log

all:
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -rf CXrefs output systemd.log log ; \
		sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc ; \
		../run-with-progress $(COMMAND) ; \
		../../utils/cxref_reader | grep -v :0:0 > output ; \
		if [ ! "$(shell uname -n)" = "thoni64" ] || diff -q output expected > /dev/null; then \
			$(OK); \
		else \
			$(FAIL); \
		fi; \
	fi

expected:
	sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc
	../../src/c-xref.orig -xrefrc .c-xrefrc -p $(PWD) -create .
	cxref_reader > expected

init: systemd

systemd:
	tar zxf systemd.tgz
