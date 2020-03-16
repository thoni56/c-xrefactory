# Makefile for ffmpeg example/acceptance test for c-xrefactory
# If the ffmpeg repo is not cloned here the test are inactive
# and will not run. To activate use 'make init' which clones the repo
# and generates the .c-xrefrc from a template. The tests will then
# run with 'make'. They can be deactivated again by 'make clean'.

all: .c-xrefrc
	@if [ ! -d ffmpeg ] ; then \
		echo "Tests in 'ffmpeg': not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		printf "Tests in 'ffmpeg': " ; \
		../run-c-xref -p ffmpeg -o ffmpeg.log ; \
		if ! grep Generating ffmpeg.log > /dev/null ; then \
			echo "$(shell tput setaf 1)Failed (ffmpeg.log)$(shell tput sgr0)" ; \
			exit 1 ; \
		else \
			if grep -i "<error " out > /dev/null ; then \
				echo "$(shell tput setaf 1)Failed (out)$(shell tput sgr0)" ; \
				exit 1 ; \
			else \
				$(OK) ; \
			fi ; \
		fi ; \
	fi

init: ffmpeg .c-xrefrc

ffmpeg:
	git clone git://source.ffmpeg.org/ffmpeg.git

.c-xrefrc: c-xrefrc.tpl
	@sed -e "s+PWD+$(CURDIR)+g" c-xrefrc.tpl > .c-xrefrc

clean:
	rm -rf CXrefs c-xref .c-xrefrc ffmpeg ffmpeg.log
