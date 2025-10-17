include ../Makefile.boilerplate

# This test ensures that c-xref have generated an entry in CXrefs/XFiles for each
# .c file in the directory - 2616 files

# It requires 47350577 (~47M) bytes of cxMemory

ARGUMENTS = -create -o ffmpeg.log

all: .c-xrefrc
	if [ ! -d ffmpeg ] ; then \
		echo "Tests in '$(TEST)': not activated, run 'make init', 'make clean' to deactivate" ; \
	else \
		rm -rf CXrefs ; \
		../run-with-progress $(COMMAND) ; \
		grep $(PWD) CXrefs/XFiles | grep \\.c | wc -l | tr -ds ' ' '' > output ; \
		$(VERIFY) ; \
	fi

run:
	echo $(COMMAND)
	$(COMMAND)

init: ffmpeg .c-xrefrc

ffmpeg:
	tar zxf ffmpeg.tgz
