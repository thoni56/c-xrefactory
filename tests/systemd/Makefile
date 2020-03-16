# Makefile for systemd example/acceptance test for c-xrefactory
# If the systemd repo is not cloned here the test are inactive
# and will not run. To activate use 'make init' which clones the repo
# and generates the .c-xrefrc from a template. The tests will then
# run with 'make'. They can be deactivated again by 'make clean'.

all:
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -f out systemd.log ; \
		printf "Tests in \"systemd\": " ; \
		../run-c-xref -p systemd -o systemd.log ; \
		if ! grep Generating systemd.log > /dev/null ; then \
			echo "$(shell tput setaf 1)Failed (systemd.log)$(shell tput sgr0)" ; \
			exit 1 ; \
		else \
			if grep -i "<error " out > /dev/null ; then \
				echo "$(shell tput setaf 1)Failed (out)$(shell tput sgr0)" ; \
				exit 1 ; \
			else \
				echo "$(shell tput setaf 2)Ok$(shell tput sgr0)" ; \
			fi ; \
		fi ; \
	fi

init: systemd .c-xrefrc

systemd:
	git clone git://anongit.freedesktop.org/systemd/systemd

.c-xrefrc: c-xrefrc.tpl
	sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > .c-xrefrc

clean:
	rm -rf CXrefs .c-xrefrc systemd systemd.log
