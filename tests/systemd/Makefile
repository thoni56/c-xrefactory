# Makefile for systemd example/acceptance test for c-xrefactory
# there should be a c-xref in this directory, either an exe or
# a link to one, presumably the development version

all:
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		printf "Tests in \"systemd\": " ; \
		./c-xref -create -xrefrc c-xrefrc -p systemd -user 1 -xrefactory-II -crlfconversion -crconversion -encoding=default -exactpositionresolve -o systemd.log > /dev/null ; \
		grep Generating systemd.log > /dev/null ; \
		if [ $$? -ne 0 ] ; then \
			echo "Failed" ; \
			exit 1 ; \
		else \
			echo "Ok" ; \
		fi ; \
	fi

init: systemd c-xref c-xrefrc

systemd:
	git clone git://anongit.freedesktop.org/systemd/systemd

c-xref:
	ln -s ../../src/c-xref

c-xrefrc: c-xrefrc.tpl
	sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > c-xrefrc

clean:
	rm -rf CXrefs c-xref c-xrefrc systemd systemd.log
