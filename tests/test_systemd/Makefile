include ../Makefile.boilerplate

ARGUMENTS = -create -o systemd.log

all: .c-xrefrc
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -f output systemd.log ; \
		../run-with-progress $(COMMAND) ; \
		if ! grep Generating systemd.log > /dev/null ; then \
			$(FAIL) "(systemd.log)" ; \
			exit 1 ; \
		else \
			if grep -i "<error " output > /dev/null ; then \
				$(FAIL) "(output)" ; \
				exit 1 ; \
			else \
				$(OK) ; \
			fi ; \
		fi ; \
	fi

init: systemd .c-xrefrc

systemd:
	tar zxf systemd.tgz
