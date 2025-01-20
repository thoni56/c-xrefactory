include ../Makefile.boilerplate

# Generate full cross-reference info for systemd
# Will dump it but only compare if we run on "thoni64"
# since cross-references includes system files

ARGUMENTS = -create -o systemd.log

all:
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -rf CXrefs output systemd.log log ; \
		sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc ; \
		../run-with-progress $(COMMAND) ; \
		../../utils/cxref_reader > CXrefs.dump ; \
		if [ "$(shell uname -n)" = "thoni64" ] && diff -q CXrefs.dump expected.dump > /dev/null; then \
			$(OK); \
		else \
			$(FAIL); \
		fi; \
	fi

expected:
	sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc
	../../src/c-xref.orig -xrefrc .c-xrefrc -p $(PWD) -create .
	cxref_reader > expected.dump

init: systemd

systemd:
	tar zxf systemd.tgz
