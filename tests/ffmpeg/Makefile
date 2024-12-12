# Makefile for ffmpeg example/acceptance test for c-xrefactory

TEST = $(shell basename '$(CURDIR)')
OK = echo "Tests in '$(TEST)': $(shell tput setaf 2)Ok$(shell tput sgr0)"
FAIL = echo "Tests in '$(TEST)': $(shell tput setaf 1)Failed$(shell tput sgr0)"


all: .c-xrefrc
	@if [ ! -d ffmpeg ] ; then \
		echo "Tests in 'ffmpeg': not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		../run-c-xref -p ffmpeg -create -o ffmpeg.log ; \
		if ! grep Generating ffmpeg.log > /dev/null ; then \
			$(FAIL) "(ffmpeg.log)" ; \
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

init: ffmpeg .c-xrefrc

ffmpeg:
	tar zxf ffmpeg.tgz

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+PWD+$(CURDIR)+g" c-xrefrc.tpl > .c-xrefrc
