include ../Makefile.boilerplate

ARGUMENTS = -create -o systemd.log

all:
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -rf CXrefs output systemd.log ; \
		sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc ; \
		../run-with-progress $(COMMAND) ; \
		cxref_reader > CXrefs.dump ; \
		if diff -q CXrefs.dump expected.dump > /dev/null ; then \
			$(OK) ; \
		else \
			$(FAIL) ; \
		fi ; \
	fi

init: systemd

systemd:
	tar zxf systemd.tgz
