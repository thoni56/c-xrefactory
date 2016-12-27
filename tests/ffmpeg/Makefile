# Makefile for ffmpeg example/acceptance test for c-xrefactory
# there should be a c-xref in this directory, either an exe or
# a link to one, presumably the development version

all:
	@if [ ! -d ffmpeg ] ; then \
		echo "Tests in \"ffmpeg\": not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		printf "Tests in \"ffmpeg\": " ; \
		./c-xref -create -xrefrc c-xrefrc -p ffmpeg -user 1 -xrefactory-II -crlfconversion -crconversion -encoding=default -exactpositionresolve -o ffmpeg.log > /dev/null ; \
		grep Generating ffmpeg.log > /dev/null ; \
		if [ $$? -ne 0 ] ; then \
			echo "Failed" ; \
			exit 1 ; \
		else \
			echo "Ok" ; \
		fi ; \
	fi

init: ffmpeg c-xref c-xrefrc

ffmpeg:
	git clone git://source.ffmpeg.org/ffmpeg.git

c-xref:
	ln -s ../../src/c-xref

c-xrefrc: c-xrefrc.tpl
	sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > c-xrefrc

clean:
	rm -rf CXrefs c-xref c-xrefrc ffmpeg ffmpeg.log
