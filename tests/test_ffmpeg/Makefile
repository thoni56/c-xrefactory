include ../Makefile.boilerplate

ARGUMENTS = -create -o ffmpeg.log

all: .c-xrefrc
	if [ ! -d ffmpeg ] ; then \
		echo "Tests in 'ffmpeg': not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		../run-with-progress $(COMMAND) ; \
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
