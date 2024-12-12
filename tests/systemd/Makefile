# Makefile for systemd example/acceptance test for c-xrefactory

TEST = $(shell basename '$(CURDIR)')
OK = echo "Tests in '$(TEST)': $(shell tput setaf 2)Ok$(shell tput sgr0)"
FAIL = echo "Tests in '$(TEST)': $(shell tput setaf 1)Failed$(shell tput sgr0)"


all: .c-xrefrc
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -f output systemd.log ; \
		../run-c-xref -p systemd -create -o systemd.log ; \
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

clean:
	rm -rf systemd

systemd:
	tar zxf systemd.tgz

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > .c-xrefrc
